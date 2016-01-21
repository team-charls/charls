//
// (C) Jan de Vaan 2007-2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

using CharLS;
using Microsoft.Win32;
using System.IO;
using System.Windows;

namespace Viewer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow
    {
        // Cache the File dialog to allow it to remember the last used directory.
        private OpenFileDialog openFileDialog;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void buttonBrowse_Click(object sender, RoutedEventArgs e)
        {
            if (openFileDialog == null)
            {
                openFileDialog = new OpenFileDialog
                {
                    InitialDirectory = "c:\\",
                    Filter = "JPEG-LS files (*.jls)|*.jls|All files (*.*)|*.*"
                };
            }

            if (!openFileDialog.ShowDialog().Value)
                return;

            textBoxPath.Text = openFileDialog.FileName;
        }

        private void buttonView_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                using (var stream = new FileStream(textBoxPath.Text, FileMode.Open, FileAccess.Read))
                {
                    image.Source = new JpegLSBitmapDecoder(stream).Frames[0];
                }
            }
            catch (FileFormatException error)
            {
                MessageBox.Show("Error: " + error.Message, Title, MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
    }
}
