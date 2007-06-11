/*
 * plugin-class.cpp: MoonLight browser plugin.
 *
 * Author:
 *   Everaldo Canuto (everaldo@novell.com)
 *
 * Copyright 2007 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#include "plugin-class.h"

/*** Static wrapper functions *************************************************/

static NPObject*
RuntimeClassAllocate (NPP instance, NPClass *aClass)
{
	NPObject *object;
	object = (NPObject*) NPN_MemAlloc (sizeof (NPObject));
	if (!object)
		return NULL;

	return object;
}

static void
RuntimeClassDeallocate (NPObject *npobj)
{
	PluginClass *plugin = (PluginClass *) npobj->_class;
	if (plugin != NULL)
		plugin->ClassDeallocate (npobj);
}

static void
RuntimeClassInvalidate (NPObject *npobj)
{
	PluginClass *plugin = (PluginClass *) npobj->_class;
	if (plugin != NULL)
		plugin->ClassInvalidate (npobj);
}

static bool
RuntimeClassHasProperty (NPObject *npobj, NPIdentifier name)
{
	PluginClass *plugin = (PluginClass *) npobj->_class;
	if (plugin != NULL)
		return plugin->ClassHasProperty (npobj, name);

	return false;
}

static bool
RuntimeClassGetProperty (NPObject *npobj, NPIdentifier name, NPVariant *result)
{
	PluginClass *plugin = (PluginClass *) npobj->_class;
	if (plugin != NULL)
		return plugin->ClassGetProperty (npobj, name, result);

	return false;
}

static bool
RuntimeClassSetProperty (NPObject *npobj, NPIdentifier name, const NPVariant *value)
{
	PluginClass *plugin = (PluginClass *) npobj->_class;
	if (plugin != NULL)
		return plugin->ClassSetProperty (npobj, name, value);

	return false;
}

static bool
RuntimeClassRemoveProperty (NPObject *npobj, NPIdentifier name)
{
	PluginClass *plugin = (PluginClass *) npobj->_class;
	if (plugin != NULL)
		return plugin->ClassRemoveProperty (npobj, name);

	return false;
}

static bool
RuntimeClassHasMethod (NPObject *npobj, NPIdentifier name)
{
	PluginClass *plugin = (PluginClass *) npobj->_class;
	if (plugin != NULL)
		return plugin->ClassHasMethod (npobj, name);

	return false;
}

static bool
RuntimeClassInvoke (NPObject *npobj, NPIdentifier name, const NPVariant *args, 
                  uint32_t argCount, NPVariant *result)
{
	PluginClass *plugin = (PluginClass *) npobj->_class;
	if (plugin != NULL)
		return plugin->ClassInvoke (npobj, name, args, argCount, result);

	return false;
}

static bool
RuntimeClassInvokeDefault (NPObject *npobj, const NPVariant *args,
	                                uint32_t argCount, NPVariant *result)
{
	PluginClass *plugin = (PluginClass *) npobj->_class;
	if (plugin != NULL)
		return plugin->ClassInvokeDefault (npobj, args, argCount, result);

	return false;
}

/*** PluginClass **************************************************************/

PluginClass::PluginClass ()
{
	this->allocate       = &RuntimeClassAllocate;
	this->deallocate     = &RuntimeClassDeallocate;
	this->invalidate     = &RuntimeClassInvalidate;
	this->hasProperty    = &RuntimeClassHasProperty;
	this->getProperty    = &RuntimeClassGetProperty;
	this->setProperty    = &RuntimeClassSetProperty;
	this->removeProperty = &RuntimeClassRemoveProperty;
	this->hasMethod      = &RuntimeClassHasMethod;
	this->invoke         = &RuntimeClassInvoke;
	this->invokeDefault  = &RuntimeClassInvokeDefault;
}

PluginClass::~PluginClass ()
{
	// nothing to do.
}

NPObject*
PluginClass::ClassAllocate (NPP instance, NPClass *aClass)
{
	NPObject *object;
	object = (NPObject*) NPN_MemAlloc (sizeof (NPObject));
	if (!object)
		return NULL;

	return object;
}

void
PluginClass::ClassDeallocate (NPObject *npobj)
{
	DEBUGMSG ("   ### Deallocate ###   ");

	// nothing to do.
}

void
PluginClass::ClassInvalidate (NPObject *npobj)
{
	// nothing to do.
}

bool
PluginClass::ClassHasProperty (NPObject *npobj, NPIdentifier name)
{
	fprintf (stderr, "*** PluginClass::ClassHasProperty (npobj=%p, name=%p)\n", npobj, name);
	return false;
}

bool
PluginClass::ClassGetProperty (NPObject *npobj, NPIdentifier name, NPVariant *result)
{
	fprintf (stderr, "*** PluginClass::ClassGetProperty\n");
	return false;
}

bool
PluginClass::ClassSetProperty (NPObject *npobj, NPIdentifier name, const NPVariant *value)
{
	fprintf (stderr, "*** PluginClass::ClassSetProperty\n");
	return false;
}

bool
PluginClass::ClassRemoveProperty (NPObject *npobj, NPIdentifier name)
{
	return false;
}

bool
PluginClass::ClassHasMethod (NPObject *npobj, NPIdentifier name)
{
	fprintf (stderr, "*** PluginClass::ClassHasMethod\n");
	return false;
}

bool
PluginClass::ClassInvoke (NPObject *npobj, NPIdentifier name, const NPVariant *args, 
                  uint32_t argCount, NPVariant *result)
{
	fprintf (stderr, "*** PluginClass::ClassInvoke\n");
	return false;
}

bool
PluginClass::ClassInvokeDefault (NPObject *npobj, const NPVariant *args,
	                                uint32_t argCount, NPVariant *result)
{
	fprintf (stderr, "*** PluginClass::ClassInvokeDefault\n");
	return false;
}

/*** PluginRootClass **********************************************************/

PluginRootClass::PluginRootClass (NPP instance)
{
	DEBUGMSG ("PluginRootClass::PluginRootClass");
	this->instance = instance;
}

bool
PluginRootClass::ClassHasProperty (NPObject *npobj, NPIdentifier name)
{
	DEBUGMSG ("PluginRootClass::ClassHasProperty");

	NPUTF8 * strname = NPN_UTF8FromIdentifier (name);
	fprintf (stderr, "-----> ClassHasProperty %s\n", strname);
	NPN_MemFree(strname);

	if (name == NPN_GetStringIdentifier ("settings")  ||
		name == NPN_GetStringIdentifier ("version"))
		return true;

	return false;
}

bool
PluginRootClass::ClassGetProperty (NPObject *npobj, NPIdentifier name, NPVariant *result)
{
	if (name == NPN_GetStringIdentifier ("settings")) 
	{
		PluginSettings *settings;
		settings = new PluginSettings ();

		NPObject *obj = NPN_CreateObject (this->instance, settings);

		OBJECT_TO_NPVARIANT (obj, *result);

		return true;
	} 
	else if (name == NPN_GetStringIdentifier ("version")) 
	{
		int len = strlen (PLUGIN_VERSION);
		char *version = (char *) NPN_MemAlloc (len + 1);
		memcpy (version, PLUGIN_VERSION, len + 1);
		STRINGN_TO_NPVARIANT (version, len, *result);

		return true;
	}

	return false;
}

/*** PluginSettings class *****************************************************/

bool
PluginSettings::ClassHasProperty (NPObject *npobj, NPIdentifier name)
{
	if (name == NPN_GetStringIdentifier ("version"))
		return true;

	return false;
}

bool
PluginSettings::ClassGetProperty (NPObject *npobj, NPIdentifier name, NPVariant *result)
{
	if (name == NPN_GetStringIdentifier ("version")) 
	{
		int len = strlen (PLUGIN_VERSION);
		char *version = (char *) NPN_MemAlloc (len + 1);
		memcpy (version, PLUGIN_VERSION, len + 1);
		STRINGN_TO_NPVARIANT (version, len, *result);

		return true;
	}

	return false;
}
