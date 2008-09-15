//
// HtmlDocument.cs
//
// Contact:
//   Moonlight List (moonlight-list@lists.ximian.com)
//
// Copyright (C) 2007 Novell, Inc (http://www.novell.com)
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
using System;
using System.Collections.Generic;
using System.Security;

namespace System.Windows.Browser
{
	public class HtmlDocument  : HtmlObject 
	{
		private HtmlElement document_element;

		internal HtmlDocument ()
		{
		}

		internal HtmlDocument (IntPtr handle)
			: base (handle)
		{
		}

		public HtmlElement DocumentElement {
			get {
				if (document_element == null)
					document_element = new HtmlElement (Handle);
				return document_element;
			}
		}

		[SecuritySafeCritical ()]
		public HtmlElement CreateElement (string tagName)
		{
			return new HtmlElement (InvokeInternal<IntPtr> (Handle, "createElement", tagName));
		}

		[SecuritySafeCritical ()]
		public HtmlElement GetElementById (string id)
		{
			return new HtmlElement (InvokeInternal<IntPtr> (Handle, "getElementById", id));
		}

		[SecuritySafeCritical ()]
		public HtmlElementCollection GetElementsByTagName (string tagName)
		{
			return new HtmlElementCollection (InvokeInternal<IntPtr> (Handle, "getElementsByTagName", tagName));
		}
		
		public IDictionary<string,string> QueryString {
			get {
				// document.location.search
				IntPtr loc = GetPropertyInternal<IntPtr> (Handle, "location");
				string search = GetPropertyInternal<string> (loc, "search");

				Dictionary<string, string> res = new Dictionary<string, string> ();

				if (search == null)
					return res;

				int li = 1;
				string name = null;
				bool in_name = true;
				for (int i = 1; i < search.Length; i++) {
					if (in_name) {
						if (search [i] == '=') {
							name = search.Substring (li, i - li);
							in_name = false;
							li = i + 1;
						}
					} else {
						if (search [i] == '&') {
							res.Add (name, search.Substring (li, i - li));
							in_name = true;
							li = i + 1;
						}
					}
				}

				if (name != null && li < search.Length)
					res.Add (name, search.Substring (li, search.Length - li));

				return res;
			}
		}
		
		public string Cookies {
			get {
				return GetPropertyInternal<string> (Handle, "cookie");
			}
			set {
				SetPropertyInternal (Handle, "cookie", value);
			}
		}

		public Uri DocumentUri {
			get {

				return new Uri (GetPropertyInternal<string> (Handle, "URL"));
			}
		}
		
		public HtmlElement Body {
			get { throw new NotImplementedException (); }
		}
		
		public void Submit ()
		{
			HtmlElementCollection forms = GetElementsByTagName ("form");
			if (forms.Count < 1)
				return;
			InvokeInternal<object> (forms [0].Handle, "submit");
		}

		public void Submit (string formId)
		{
			HtmlElement form = GetElementById (formId);
			if (form == null)
				return;
			InvokeInternal<object> (form.Handle, "submit");
		}
		
		public bool IsReady {
			get { throw new NotImplementedException (); }
		}
		
		
		public event EventHandler DocumentReady;
	}
}

