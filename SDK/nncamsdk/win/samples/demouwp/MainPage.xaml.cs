using System;
using System.Collections.Generic;
using System.IO;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Popups;
using Windows.UI.Core;
using Windows.ApplicationModel.Core;
using Windows.UI.Xaml.Media.Imaging;
using Windows.Graphics.Imaging;
using System.Runtime.InteropServices;

namespace demouwp
{
    [ComImport]
    [Guid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    unsafe interface IMemoryBufferByteAccess
    {
        void GetBuffer(out byte* buffer, out uint capacity);
    }

    public sealed partial class MainPage : Page
    {
        private Toupcam cam_;
        private Toupcam.DeviceV2[] arr_;
        private SoftwareBitmap bmpfront_;
        private SoftwareBitmap bmpback_;
        private uint frm_;

        public MainPage()
        {
            this.InitializeComponent();
            wbonepush_.IsEnabled = false;
            autoexposure_.IsChecked = true;

            frm_ = 0;
            arr_ = Toupcam.EnumV2();
            List<string> items = new List<string>();
            if (arr_.Length <= 0)
                items.Add("No Device");
            else
            {
                for (int i = 0; i < arr_.Length; ++i)
                    items.Add(arr_[i].displayname);
            }
            combobox_.ItemsSource = items;

            if (arr_.Length <= 0)
            {
                combobox_.SelectedIndex = 0;
                combobox_.IsEnabled = false;
                rescombo_.IsEnabled = false;
            }
        }

        private void CloseCamera()
        {
            cam_?.Close();
            cam_ = null;
            wbonepush_.IsEnabled = false;
            rescombo_.IsEnabled = false;
        }

        private void StartCamera()
        {
            frm_ = 0;

            cam_.put_AutoExpoEnable(autoexposure_.IsChecked == true);
            uint exptime = 0;
            cam_.get_ExpoTime(out exptime);
            exptime_.Text = "ExpTime: " + exptime.ToString();
            frames_.Text = "Frames: 0";
            int width = 0, height = 0;
            cam_.get_Size(out width, out height);
            bmpfront_ = new SoftwareBitmap(BitmapPixelFormat.Bgra8, width, height, BitmapAlphaMode.Ignore);
            bmpback_ = new SoftwareBitmap(BitmapPixelFormat.Bgra8, width, height, BitmapAlphaMode.Ignore);
            if (!cam_.StartPullModeWithCallback(new Toupcam.DelegateEventCallback(DelegateOnEventCallback)))
            {
                CloseCamera();
                bmpfront_ = bmpback_ = null;
            }
        }

        private void DelegateOnEventCallback(Toupcam.eEVENT ev)
        {
            /* http://stackoverflow.com/questions/16477190/correct-way-to-get-the-coredispatcher-in-a-windows-store-app */
            var ignore = CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(CoreDispatcherPriority.Normal,
            async () =>
            {
                if (ev == Toupcam.eEVENT.EVENT_EXPOSURE)
                {
                    uint exptime = 0;
                    cam_.get_ExpoTime(out exptime);
                    exptime_.Text = "ExpTime: " + exptime.ToString();
                }
                else if (ev == Toupcam.eEVENT.EVENT_DISCONNECTED)
                {
                    CloseCamera();
                    var msgDialog = new Windows.UI.Popups.MessageDialog("Device disconnected") { Title = "Error" };
                    msgDialog.Commands.Add(new Windows.UI.Popups.UICommand("OK", uiCommand => { }));
                    await msgDialog.ShowAsync();
                }
                else if (ev == Toupcam.eEVENT.EVENT_ERROR)
                {
                    CloseCamera();
                    var msgDialog = new Windows.UI.Popups.MessageDialog("Device error") { Title = "Error" };
                    msgDialog.Commands.Add(new Windows.UI.Popups.UICommand("OK", uiCommand => { }));
                    await msgDialog.ShowAsync();
                }
                else if (ev == Toupcam.eEVENT.EVENT_IMAGE)
                {
                    bool set = false;
                    uint width = 0, height = 0;

                    using (BitmapBuffer buffer = bmpback_.LockBuffer(BitmapBufferAccessMode.Write))
                    using (IMemoryBufferReference reference = buffer.CreateReference())
                    {
                        unsafe
                        {
                            byte* dataInBytes;
                            uint capacity;
                            ((IMemoryBufferByteAccess)reference).GetBuffer(out dataInBytes, out capacity);
                            if (cam_.PullImage((IntPtr)dataInBytes, 32, out width, out height))
                                set = true;
                        }
                    }

                    if (set)
                    {
                        ++frm_;
                        frames_.Text = "Frames: " + frm_.ToString();
                        SoftwareBitmapSource imageSource = (SoftwareBitmapSource)image_.Source;
                        SoftwareBitmap bmpt = bmpback_;
                        bmpback_ = bmpfront_;
                        bmpfront_ = bmpt;
                        try
                        {
                            await imageSource.SetBitmapAsync(bmpt);
                        }catch (Operation​Canceled​Exception) {}
                    }
                }
            });
        }
		
        private void OnDeviceChanged(object sender, SelectionChangedEventArgs e)
        {
            if ((arr_.Length <= 0) || (cam_ != null))
                return;
           
            image_.Source = new SoftwareBitmapSource();
            cam_ = Toupcam.Open(arr_[combobox_.SelectedIndex].id);
            if (cam_ != null)
            {
                List<string> items = new List<string>();
                for (uint i = 0; i < arr_[combobox_.SelectedIndex].model.preview; ++i)
                    items.Add(arr_[combobox_.SelectedIndex].model.res[i].width.ToString() + "x" + arr_[combobox_.SelectedIndex].model.res[i].height.ToString());
                rescombo_.ItemsSource = items;
                
                uint eres = 0;
                cam_.get_eSize(out eres);
                rescombo_.SelectedIndex = (int)eres;

                rescombo_.IsEnabled = true;
                wbonepush_.IsEnabled = true;
            }
        }

        private void OnResChanged(object sender, SelectionChangedEventArgs e)
        {
            cam_.Stop();
            bmpfront_ = bmpback_ = null;

            cam_.put_eSize((uint)rescombo_.SelectedIndex);
            StartCamera();
        }

        private void OnWhiteBalance(object sender, RoutedEventArgs e)
        {
            cam_?.AwbOnePush();
        }

        private void OnAutoExposure(object sender, RoutedEventArgs e)
        {
            cam_?.put_AutoExpoEnable(autoexposure_.IsChecked == true);
        }
    }
}
