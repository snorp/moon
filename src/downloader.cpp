/*
 * runtime.cpp: Downloader class.
 *
 * The downloader implements two modes of operation:
 *
 *    bare bones:  this is the interface expected by Javascript and C#
 *		 this is the default if the caller does not call
 *		 Downloader::SetWriteFunc
 * 
 *    progressive: this interface is used internally by the Image
 *		 class to do progressive loading.   If you want to
 *		 use this mode, you must call the SetWriteFunc routine
 *		 to install your callbacks before starting the download.
 * 
 * TODO:
 *    Need a mechanism to notify the managed client of errors during 
 *    download.
 *
 *    Need to provide the buffer we downloaded to GetResponseText(string PartName)
 *    so we can return the response text for the given part name.
 *
 *    The providers should store the files *somewhere* and should be able
 *    to respond to the "GetResponsetext" above on demand.   The current
 *    code in demo.cpp and ManagedDownloader are not complete in this regard as
 *    they only stream
 *
 * Author:
 *   Chris Toshok (toshok@novell.com)
 *   Miguel de Icaza (miguel@novell.com).
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "downloader.h"
#include "file-downloader.h"
#include "mms-downloader.h"
#include "zip/unzip.h"
#include "runtime.h"
#include "utils.h"
#include "error.h"

//
// MMS
//

#define LOG_MMS(...) //printf (__VA_ARGS__);

static inline bool
is_valid_mms_header (MmsHeader *header)
{
	if (header->id != MMS_DATA && header->id != MMS_HEADER && header->id != MMS_METADATA && header->id != MMS_STREAM_C && header->id != MMS_END && header->id != MMS_PAIR_P)
		return false;

	return true;
}

//
// Downloader
//

downloader_create_state_func Downloader::create_state = NULL;
downloader_destroy_state_func Downloader::destroy_state = NULL;
downloader_open_func Downloader::open_func = NULL;
downloader_send_func Downloader::send_func = NULL;
downloader_abort_func Downloader::abort_func = NULL;
downloader_header_func Downloader::header_func = NULL;
downloader_body_func Downloader::body_func = NULL;

DependencyProperty *Downloader::DownloadProgressProperty;
DependencyProperty *Downloader::ResponseTextProperty;
DependencyProperty *Downloader::StatusProperty;
DependencyProperty *Downloader::StatusTextProperty;
DependencyProperty *Downloader::UriProperty;

Downloader::Downloader ()
{
	downloader_state = Downloader::create_state (this);
	consumer_closure = NULL;
	context = NULL;
	streaming_features = HttpStreamingFeaturesNone;
	notify_size = NULL;
	this->write = NULL;
	request_position = NULL;
	internal_dl = NULL;
	
	deobfuscated = false;
	send_queued = false;
	unlinkit = false;
	unzipped = false;
	started = false;
	aborted = false;
	file_size = -2;
	total = 0;
	
	failed_msg = NULL;
	filename = NULL;
	unzipdir = NULL;
}

void
Downloader::CleanupUnzipDir ()
{
	if (!unzipdir)
		return;
	
	RemoveDir (unzipdir);
	g_free (unzipdir);
	unzipped = false;
	unzipdir = NULL;
}

Downloader::~Downloader ()
{
	Downloader::destroy_state (downloader_state);
	
	if (filename) {
		if (unlinkit)
			unlink (filename);
		g_free (filename);
	}
	
	g_free (failed_msg);
	
	// Delete temporary files.
	CleanupUnzipDir ();
}

void
Downloader::InternalAbort ()
{
	abort_func (downloader_state);
}

void
Downloader::Abort ()
{
	if (!aborted && !failed_msg) {
		InternalAbort ();
		SetDownloadProgress (0.0);
		send_queued = false;
		aborted = true;
	}
}

char *
Downloader::GetResponseText (const char *PartName, uint64_t *size)
{
	TextStream *stream;
	char buffer[4096];
	GByteArray *buf;
	struct stat st;
	ssize_t nread;
	char *data;
	char *path;
	
	if (!(path = GetDownloadedFilePart (PartName)))
		return NULL;
	
	if (stat (path, &st) == -1) {
		g_free (path);
		return NULL;
	}
	
	if (st.st_size > 0) {
		stream = new TextStream ();
		
		if (!stream->Open (path, true)) {
			delete stream;
			g_free (path);
			return NULL;
		}
		
		g_free (path);
		
		buf = g_byte_array_new ();
		while ((nread = stream->Read (buffer, sizeof (buffer))) > 0)
			g_byte_array_append (buf, (const guint8 *) buffer, nread);
		
		*size = buf->len;
		
		g_byte_array_append (buf, (const guint8 *) "", 1);
		data = (char *) buf->data;
		
		g_byte_array_free (buf, false);
		delete stream;
	} else {
		data = g_strdup ("");
		*size = 0;
	}
	
	return data;
}

const char *
Downloader::GetDownloadedFile ()
{
	return filename;
}

bool
Downloader::DownloadedFileIsZipped ()
{
	unzFile zipfile;
	
	if (!filename)
		return false;
	
	if (!(zipfile = unzOpen (filename)))
		return false;
	
	unzClose (zipfile);
	
	return true;
}

char *
Downloader::GetDownloadedFilePart (const char *PartName)
{
	char *dirname, *path, *part;
	unzFile zipfile;
	struct stat st;
	int rv, fd;
	
	if (!filename)
		return NULL;
	
	if (!PartName || !PartName[0])
		return g_strdup (filename);
	
	if (!DownloadedFileIsZipped ())
		return NULL;
	
	if (!unzipdir && !(unzipdir = CreateTempDir (filename)))
		return NULL;
	
	part = g_ascii_strdown (PartName, -1);
	path = g_build_filename (unzipdir, part, NULL);
	if ((rv = stat (path, &st)) == -1 && errno == ENOENT) {
		if (strchr (part, '/') != NULL) {
			// create the directory path
			dirname = g_path_get_dirname (path);
			rv = g_mkdir_with_parents (dirname, 0700);
			g_free (dirname);
			
			if (rv == -1 && errno != EEXIST)
				goto exception1;
		}
		
		// open the zip archive...
		if (!(zipfile = unzOpen (filename)))
			goto exception1;
		
		// locate the file we want to extract... (2 = case-insensitive)
		if (unzLocateFile (zipfile, PartName, 2) != UNZ_OK)
			goto exception2;
		
		// open the requested part within the zip file
		if (unzOpenCurrentFile (zipfile) != UNZ_OK)
			goto exception2;
		
		// open the output file
		if ((fd = open (path, O_CREAT | O_WRONLY, 0644)) == -1)
			goto exception3;
		
		// extract the file from the zip archive... (closes the fd on success and fail)
		if (!ExtractFile (zipfile, fd))
			goto exception3;
		
		unzCloseCurrentFile (zipfile);
		unzClose (zipfile);
	} else if (rv == -1) {
		// irrecoverable error
		goto exception0;
	}
	
	g_free (part);
	
	return path;
	
exception3:
	
	unzCloseCurrentFile (zipfile);
	
exception2:
	
	unzClose (zipfile);
	
exception1:
	
	g_free (part);
	
exception0:
	
	g_free (path);
	
	return NULL;
}

const char *
Downloader::GetUnzippedPath ()
{
	char filename[256], *p;
	unz_file_info info;
	const char *name;
	GString *path;
	unzFile zip;
	size_t len;
	int fd;
	
	if (!this->filename)
		return NULL;
	
	if (!DownloadedFileIsZipped ())
		return this->filename;
	
	if (!unzipdir && !(unzipdir = CreateTempDir (this->filename)))
		return NULL;
	
	if (unzipped)
		return unzipdir;
	
	// open the zip archive...
	if (!(zip = unzOpen (this->filename)))
		return NULL;
	
	path = g_string_new (unzipdir);
	g_string_append_c (path, G_DIR_SEPARATOR);
	len = path->len;
	
	unzipped = true;
	
	// extract all the parts
	do {
		if (unzOpenCurrentFile (zip) != UNZ_OK)
			break;
		
		unzGetCurrentFileInfo (zip, &info, filename, sizeof (filename),
				       NULL, 0, NULL, 0);
		
		// convert filename to lowercase
		for (p = filename; *p; p++) {
			if (*p >= 'A' && *p <= 'Z')
				*p += 0x20;
		}
		
		if ((name = strrchr (filename, '/'))) {
			// make sure the full directory path exists, if not create it
			g_string_append_len (path, filename, name - filename);
			g_mkdir_with_parents (path->str, 0700);
			g_string_append (path, name);
		} else {
			g_string_append (path, filename);
		}
		
		if ((fd = open (path->str, O_WRONLY | O_CREAT | O_EXCL, 0600)) != -1) {
			if (!ExtractFile (zip, fd))
				unzipped = false;
		} else if (errno != EEXIST) {
			unzipped = false;
		}
		
		g_string_truncate (path, len);
		unzCloseCurrentFile (zip);
	} while (unzGoToNextFile (zip) == UNZ_OK);
	
	g_string_free (path, true);
	unzClose (zip);
	
	return unzipdir;
}

bool
Downloader::IsDeobfuscated ()
{
	return deobfuscated;
}

void
Downloader::SetDeobfuscated (bool val)
{
	deobfuscated = val;
}

void
Downloader::SetDeobfuscatedFile (const char *path)
{
	if (!filename || !path)
		return;
	
	if (deobfuscated)
		unlink (filename);
	g_free (filename);
	
	filename = g_strdup (path);
	deobfuscated = true;
	unlinkit = true;
}

void
Downloader::InternalOpen (const char *verb, const char *uri, bool streaming)
{
	open_func (verb, uri, streaming, downloader_state);
}

void
Downloader::Open (const char *verb, const char *uri)
{
	char *internal_uri;

	CleanupUnzipDir ();
	
	if (filename) {
		if (unlinkit)
			unlink (filename);
		g_free (filename);
	}
	
	deobfuscated = false;
	send_queued = false;
	unlinkit = false;
	unzipped = false;
	started = false;
	aborted = false;
	mms = false;
	file_size = -2;
	total = 0;
	
	g_free (failed_msg);
	failed_msg = NULL;
	filename = NULL;
	
	if (strncmp (uri, "mms://", 6) == 0) {
		internal_dl = (InternalDownloader *) new MmsDownloader (this);
	} else {
		internal_dl = (InternalDownloader *) new FileDownloader (this);
	}

	SetUri (uri);

	internal_dl->Open (verb, uri);
}

void
Downloader::InternalSetHeader (const char *header, const char *value)
{
	header_func (downloader_state, header, value);
}

void
Downloader::InternalSetBody (void *body, uint32_t length)
{
	body_func (downloader_state, body, length);
}

void
Downloader::SendInternal ()
{
	if (!GetSurface ()) {
		// The plugin is already checking for surface before calling Send, so
		// if we get here, it's either managed code doing something wrong or ourselves.
		g_warning ("Downloader::SendInternal (): No surface!\n");
	}

	if (!send_queued)
		return;
	
	send_queued = false;
	
	if (filename != NULL) {
		// Consumer is re-sending a request which finished successfully.
		NotifyFinished (filename);
		return;
	}
	
	if (failed_msg != NULL) {
		// Consumer is re-sending a request which failed.
		Emit (DownloadFailedEvent, new ErrorEventArgs (DownloadError, 1, failed_msg));
		return;
	}
	
	started = true;
	aborted = false;
	
	send_func (downloader_state);
}

static void
send_async (void *user_data)
{
	Downloader *downloader = (Downloader *) user_data;
	
	downloader->SendInternal ();
	downloader->unref ();
}

void
Downloader::Send ()
{
	if (!GetSurface ()) {
		// The plugin is already checking for surface before calling Send, so
		// if we get here, it's either managed code doing something wrong or ourselves.
		g_warning ("Downloader::Send (): No surface!\n");
	}

	if (send_queued)
		return;
	
	send_queued = true;
	SetStatusText ("");
	SetStatus (0);
	
	AddTickCall (send_async);
}

void
Downloader::SendNow ()
{
	send_queued = true;
	SetStatusText ("");
	SetStatus (0);
	
	SendInternal ();
}

//
// A zero write means that we are done
//
void
Downloader::Write (void *buf, int32_t offset, int32_t n)
{
	if (aborted)
		return;
		
	if (!GetSurface ())
		return;
	
	internal_dl->Write (buf, offset, n);
}

void
Downloader::InternalWrite (void *buf, int32_t offset, int32_t n)
{
	double progress;

	// Update progress
	if (n > 0)
		total += n;

	if (file_size >= 0) {
		if ((progress = total / (double) file_size) > 1.0)
			progress = 1.0;
	} else 
		progress = 0.0;

	SetDownloadProgress (progress);
	
	Emit (DownloadProgressChangedEvent);

	if (write)
		write (buf, offset, n, consumer_closure);
}

void
Downloader::RequestPosition (int64_t *pos)
{
	if (aborted)
		return;
	
	if (request_position)
		request_position (pos, consumer_closure);
}


void
Downloader::NotifyFinished (const char *fname)
{
	if (aborted)
		return;
	
	if (!GetSurface ())
		return;
	
	filename = g_strdup (fname);
	
	SetDownloadProgress (1.0);
	
	Emit (DownloadProgressChangedEvent);
	
	// HACK, we should provide the actual status text and code
	SetStatusText ("OK");
	SetStatus (200);
	
	Emit (CompletedEvent, NULL);
}

void
Downloader::NotifyFailed (const char *msg)
{
	/* if we've already been notified of failure, no-op */
	if (failed_msg)
		return;
	
	if (!GetSurface ())
		return;
	
	// SetStatus (400);
	// For some reason the status is 0, not updated on errors?
	
	Emit (DownloadFailedEvent, new ErrorEventArgs (DownloadError, 1, msg));
	
	// save the error in case someone else calls ::Send() on this
	// downloader for the same uri.
	failed_msg = g_strdup (msg);
}

void
Downloader::NotifySize (int64_t size)
{
	file_size = size;
	
	if (aborted)
		return;
	
	if (!GetSurface ())
		return;
	
	if (notify_size)
		notify_size (size, consumer_closure);
}

bool
Downloader::Started ()
{
	return started;
}

bool
Downloader::Completed ()
{
	return filename != NULL;
}

void
Downloader::SetWriteFunc (downloader_write_func write,
			  downloader_notify_size_func notify_size,
			  gpointer data)
{
	this->write = write;
	this->notify_size = notify_size;
	this->consumer_closure = data;
}

void
Downloader::SetFunctions (downloader_create_state_func create_state,
			  downloader_destroy_state_func destroy_state,
			  downloader_open_func open,
			  downloader_send_func send,
			  downloader_abort_func abort,
			  downloader_header_func header,
			  downloader_body_func body,
			  bool only_if_not_set)
{
	if (only_if_not_set &&
	    (Downloader::create_state != NULL ||
	     Downloader::destroy_state != NULL ||
	     Downloader::open_func != NULL ||
	     Downloader::send_func != NULL ||
	     Downloader::abort_func != NULL ||
	     Downloader::header_func != NULL ||
	     Downloader::body_func != NULL))
	  return;

	Downloader::create_state = create_state;
	Downloader::destroy_state = destroy_state;
	Downloader::open_func = open;
	Downloader::send_func = send;
	Downloader::abort_func = abort;
	Downloader::header_func = header;
	Downloader::body_func = body;
}

void
Downloader::SetRequestPositionFunc (downloader_request_position_func request_position)
{
       this->request_position = request_position;
}

void
Downloader::SetDownloadProgress (double progress)
{
	SetValue (Downloader::DownloadProgressProperty, Value (progress));
}

double
Downloader::GetDownloadProgress ()
{
	return GetValue (Downloader::DownloadProgressProperty)->AsDouble ();
}

void
Downloader::SetStatusText (const char *text)
{
	SetValue (Downloader::StatusTextProperty, Value (text));
}

const char *
Downloader::GetStatusText ()
{
	Value *value = GetValue (Downloader::StatusTextProperty);
	
	return value ? value->AsString () : NULL;
}

void
Downloader::SetStatus (int status)
{
	SetValue (Downloader::StatusProperty, Value (status));
}

int
Downloader::GetStatus ()
{
	return GetValue (Downloader::StatusProperty)->AsInt32 ();
}

void
Downloader::SetUri (const char *uri)
{
	SetValue (Downloader::UriProperty, Value (uri));

}

const char *
Downloader::GetUri ()
{
	Value *value = GetValue (Downloader::UriProperty);
	
	return value ? value->AsString () : NULL;
}

Downloader *
downloader_new (void)
{
	return new Downloader ();
}

double
downloader_get_download_progress (Downloader *dl)
{
	return dl->GetDownloadProgress ();
}

const char *
downloader_get_status_text (Downloader *dl)
{
	return dl->GetStatusText ();
}

int
downloader_get_status (Downloader *dl)
{
	return dl->GetStatus ();
}

void
downloader_set_uri (Downloader *dl, const char *uri)
{
	dl->SetUri (uri);
}

const char *
downloader_get_uri (Downloader *dl)
{
	return dl->GetUri ();
}

void
downloader_abort (Downloader *dl)
{
	dl->Abort ();
}

//
// Returns the filename that holds that given file.
// 
// Return values:
//   A newly allocated string containing the filename.
//
char *
downloader_get_downloaded_file (Downloader *dl)
{
	return g_strdup (dl->GetDownloadedFile ());
}


char *
downloader_get_response_text (Downloader *dl, const char *PartName, uint64_t *size)
{
	return dl->GetResponseText (PartName, size);
}

void
downloader_open (Downloader *dl, const char *verb, const char *uri)
{
	dl->Open (verb, uri);
}

void
downloader_send (Downloader *dl)
{
	if (!dl->Completed () && dl->Started ())
		downloader_abort (dl);
	
	dl->Send ();
}

void
downloader_set_functions (downloader_create_state_func create_state,
			  downloader_destroy_state_func destroy_state,
			  downloader_open_func open,
			  downloader_send_func send,
			  downloader_abort_func abort, 
			  downloader_header_func header,
			  downloader_body_func body)
{
	Downloader::SetFunctions (create_state, destroy_state,
				  open, send, abort, header, body, false);
}

void
downloader_request_position (Downloader *dl, int64_t *pos)
{
	dl->RequestPosition (pos);
}

void
downloader_write (Downloader *dl, void *buf, int32_t offset, int32_t n)
{
	dl->Write (buf, offset, n);
}

void
downloader_notify_finished (Downloader *dl, const char *fname)
{
	dl->NotifyFinished (fname);
}

void
downloader_notify_error (Downloader *dl, const char *msg)
{
	dl->NotifyFailed (msg);
}

void
downloader_notify_size (Downloader *dl, int64_t size)
{
	dl->NotifySize (size);
}


static gpointer
dummy_downloader_create_state (Downloader* dl)
{
	g_warning ("downloader_set_function has never been called.\n");
	return NULL;
}

static void
dummy_downloader_destroy_state (gpointer state)
{
	g_warning ("downloader_set_function has never been called.\n");
}

static void
dummy_downloader_open (const char *verb, const char *uri, bool open, gpointer state)
{
	g_warning ("downloader_set_function has never been called.\n");
}

static void
dummy_downloader_send (gpointer state)
{
	g_warning ("downloader_set_function has never been called.\n");
}

static void
dummy_downloader_abort (gpointer state)
{
	g_warning ("downloader_set_function has never been called.\n");
}

static void
dummy_downloader_header (gpointer state, const char *header, const char *value)
{
	g_warning ("downloader_set_function has never been called.\n");
}

static void
dummy_downloader_body (gpointer state, void *body, uint32_t length)
{
	g_warning ("downloader_set_function has never been called.\n");
}


void
downloader_init (void)
{
	Downloader::DownloadProgressProperty = DependencyObject::Register (Type::DOWNLOADER, "DownloadProgress", new Value (0.0));
	Downloader::ResponseTextProperty = DependencyObject::Register (Type::DOWNLOADER, "ResponseText", Type::STRING);
	Downloader::StatusProperty = DependencyObject::Register (Type::DOWNLOADER, "Status", new Value (0));
	Downloader::StatusTextProperty = DependencyObject::Register (Type::DOWNLOADER, "StatusText", new Value (""));
	Downloader::UriProperty = DependencyObject::Register (Type::DOWNLOADER, "Uri", Type::STRING);
		
	Downloader::SetFunctions (dummy_downloader_create_state,
				  dummy_downloader_destroy_state,
				  dummy_downloader_open,
				  dummy_downloader_send,
				  dummy_downloader_abort,
				  dummy_downloader_header,
				  dummy_downloader_body, true);
}
