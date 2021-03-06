using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;

using Mono.Moonlight.UnitTesting;
using Microsoft.Silverlight.Testing;
using Microsoft.VisualStudio.TestTools.UnitTesting;

// FIXME: the exceptions shouldn't be NRE's, but I'm not sure what
// they will be so amde them something they won't be so that the
// Assert fails so I can fix them.

namespace MoonTest.System.Windows.Media.Imaging {
	[TestClass]
	public partial class BitmapImageTest : SilverlightTest {
		static Uri corruptImage = new Uri ("images/invalid-image-data.png", UriKind.Relative);
		static Uri badUri = new Uri ("non-existent-uri.png", UriKind.Relative);
		
		[TestMethod]
		public void Ctor_Empty ()
		{
			BitmapImage bi = new BitmapImage ();
			Assert.AreEqual (String.Empty, bi.UriSource.OriginalString, "UriSource");
		}

		[TestMethod]
		public void Ctor_Null ()
		{
			BitmapImage bi = new BitmapImage (null);
			Assert.AreEqual (String.Empty, bi.UriSource.OriginalString, "UriSource");
		}

		// Invalid/bad Uri
		
		[TestMethod]
		public void BadUriInCtor ()
		{
			Assert.IsNotNull(new BitmapImage (badUri));
		}

		[TestMethod]
		public void Defaults ()
		{
			BitmapImage image = new BitmapImage ();
			Assert.IsFalse (image.UriSource.IsAbsoluteUri, "#1");
			Assert.AreEqual (string.Empty, image.UriSource.ToString (), "#2");
		}

		[TestMethod]
		public void EmptyUriInCtor ()
		{
			var bitmap = new BitmapImage (new Uri ("", UriKind.Relative));
			Assert.AreEqual (String.Empty, bitmap.UriSource.OriginalString, "UriSource");
			var image = new Image ();
			image.Source = bitmap;
		}

		[TestMethod]
		public void BadUriSetUriSource ()
		{
			BitmapImage bitmap = new BitmapImage ();
			bitmap.UriSource = badUri;
		}

		[TestMethod]
		public void BadUriSetUriSourceTwice ()
		{
			BitmapImage bitmap = new BitmapImage ();
			bitmap.UriSource = badUri;
			// call into BitmapImage::UriSourceChanged twice (leaks)
			bitmap.UriSource = badUri;
		}
		
		[TestMethod]
		public void BadUriSetUriSourceProperty ()
		{
			BitmapImage bitmap = new BitmapImage ();
			bitmap.SetValue (BitmapImage.UriSourceProperty, badUri);
		}
		
		
		// Corrupt image
		
		[TestMethod]
		public void CorruptImageInCtor ()
		{
			Assert.IsNotNull(new BitmapImage (corruptImage));
		}
		
		[TestMethod]
		public void CorruptImageSetUriSource ()
		{
			BitmapImage bitmap = new BitmapImage ();
			bitmap.UriSource = corruptImage;
		}
		
		[TestMethod]
		public void CorruptImageSetUriSourceProperty ()
		{
			BitmapImage bitmap = new BitmapImage ();
			bitmap.SetValue (BitmapImage.UriSourceProperty, corruptImage);
		}

		[TestMethod]
		[Asynchronous]
		public void TestUris ()
		{
			Image image = new Image ();
			TestPanel.Children.Add (image);

			//bool l1 = false;
			BitmapImage bi;

			//bi = new BitmapImage (new Uri ("http://192.168.42.20:8080/site/ClientBin/images/mono-logo.png", UriKind.RelativeOrAbsolute));
			//image.Source = bi;
			//bi.ImageOpened += delegate { l1 = true; };
			//EnqueueConditional (() => l1, "#1");

			bool l3 = false;
			bi = new BitmapImage (new Uri ("/images/mono-logo.png", UriKind.RelativeOrAbsolute));
			image.Source = bi;
			bi.ImageOpened += delegate { l3 = true; };
			EnqueueConditional (() => l3, "#3");

			//bool l5 = false;
			//bi = new BitmapImage (new Uri ("images/mono-logo.png", UriKind.RelativeOrAbsolute));
			//image.Source = bi;
			//bi.ImageFailed += delegate { l5 = true; };
			//EnqueueConditional (() => l5, "#5");

			bool l7 = false;
			bi = new BitmapImage (new Uri ("/site/ClientBin/images/mono-logo.png", UriKind.RelativeOrAbsolute));
			image.Source = bi;
			bi.ImageFailed += delegate { l7 = true; };
			EnqueueConditional (() => l7, "#7");

			//bool l9 = false;
			//bi = new BitmapImage (new Uri ("./images/mono-logo.png", UriKind.RelativeOrAbsolute));
			//image.Source = bi;
			//bi.ImageFailed += delegate { l9 = true; };
			//EnqueueConditional (() => l9, "#9");

			EnqueueTestComplete ();
		}
	}
}
