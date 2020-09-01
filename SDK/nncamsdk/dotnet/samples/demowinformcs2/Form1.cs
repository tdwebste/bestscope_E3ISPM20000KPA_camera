using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace demowinformcs2
{
    public partial class Form1 : Form
    {
        private delegate void DelegateEvent(Toupcam.eEVENT[] ev);
        private Toupcam cam_ = null;
        private Bitmap bmp_ = null;
        private DelegateEvent ev_ = null;

        private void OnEventError()
        {
            if (cam_ != null)
            {
                cam_.Close();
                cam_ = null;
            }
            MessageBox.Show("Error");
        }

        private void OnEventDisconnected()
        {
            if (cam_ != null)
            {
                cam_.Close();
                cam_ = null;
            }
            MessageBox.Show("The camera is disconnected, maybe has been pulled out.");
        }

        private void OnEventExposure()
        {
            if (cam_ != null)
            {
                uint nTime = 0;
                if (cam_.get_ExpoTime(out nTime))
                {
                    trackBar1.Value = (int)nTime;
                    label1.Text = (nTime / 1000).ToString() + " ms";
                }
            }
        }

        private void OnEventImage()
        {
            if (bmp_ != null)
            {
                BitmapData bmpdata = bmp_.LockBits(new Rectangle(0, 0, bmp_.Width, bmp_.Height), ImageLockMode.WriteOnly, bmp_.PixelFormat);

                Toupcam.FrameInfoV2 info = new Toupcam.FrameInfoV2();
                cam_.PullImageV2(bmpdata.Scan0, 24, out info);

                bmp_.UnlockBits(bmpdata);

                pictureBox1.Image = bmp_;
                pictureBox1.Invalidate();
            }
        }

        private void OnEventStillImage()
        {
            Toupcam.FrameInfoV2 info = new Toupcam.FrameInfoV2(); ;
            if (cam_.PullStillImageV2(IntPtr.Zero, 24, out info))   /* peek the width and height */
            {
                Bitmap sbmp = new Bitmap((int)info.width, (int)info.height, PixelFormat.Format24bppRgb);

                BitmapData bmpdata = sbmp.LockBits(new Rectangle(0, 0, sbmp.Width, sbmp.Height), ImageLockMode.WriteOnly, sbmp.PixelFormat);
                cam_.PullStillImageV2(bmpdata.Scan0, 24, out info);
                sbmp.UnlockBits(bmpdata);

                sbmp.Save("demowinformcs2.jpg");
            }
        }

        public Form1()
        {
            InitializeComponent();
            pictureBox1.Width = ClientRectangle.Right - button1.Bounds.Right - 20;
            pictureBox1.Height = ClientRectangle.Height - 8;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            button2.Enabled = false;
            button3.Enabled = false;
            trackBar1.Enabled = false;
            trackBar2.Enabled = false;
            trackBar3.Enabled = false;
            checkBox1.Enabled = false;
            comboBox1.Enabled = false;
        }

        private void DelegateOnEvent(Toupcam.eEVENT[] ev)
        {
            switch (ev[0])
            {
                case Toupcam.eEVENT.EVENT_ERROR:
                    OnEventError();
                    break;
                case Toupcam.eEVENT.EVENT_DISCONNECTED:
                    OnEventDisconnected();
                    break;
                case Toupcam.eEVENT.EVENT_EXPOSURE:
                    OnEventExposure();
                    break;
                case Toupcam.eEVENT.EVENT_IMAGE:
                    OnEventImage();
                    break;
                case Toupcam.eEVENT.EVENT_STILLIMAGE:
                    OnEventStillImage();
                    break;
                case Toupcam.eEVENT.EVENT_TEMPTINT:
                    OnEventTempTint();
                    break;
            }
        }

        private void DelegateOnEventCallback(Toupcam.eEVENT ev)
        {
            /* this delegate is call by internal thread of toupcam.dll which is NOT the same of UI thread.
             * Why we use BeginInvoke, Please see:
             * http://msdn.microsoft.com/en-us/magazine/cc300429.aspx
             * http://msdn.microsoft.com/en-us/magazine/cc188732.aspx
             * http://stackoverflow.com/questions/1364116/avoiding-the-woes-of-invoke-begininvoke-in-cross-thread-winform-event-handling
            */
            BeginInvoke(ev_, new Toupcam.eEVENT[1] { ev });
        }

        private void OnStart(object sender, EventArgs e)
        {
            if (cam_ != null)
                return;

            Toupcam.DeviceV2[] arr = Toupcam.EnumV2();
            if (arr.Length <= 0)
                MessageBox.Show("no device");
            else
            {
                cam_ = Toupcam.Open(arr[0].id);
                if (cam_ != null)
                {
                    checkBox1.Enabled = true;
                    trackBar1.Enabled = true;
                    trackBar2.Enabled = true;
                    trackBar3.Enabled = true;
                    comboBox1.Enabled = true;
                    button2.Enabled = true;
                    button3.Enabled = true;
                    button2.ContextMenuStrip = null;
                    InitSnapContextMenuAndExpoTimeRange();

                    trackBar2.SetRange(2000, 15000);
                    trackBar3.SetRange(200, 2500);
                    OnEventTempTint();

                    uint resnum = cam_.ResolutionNumber;
                    uint eSize = 0;
                    if (cam_.get_eSize(out eSize))
                    {
                        for (uint i = 0; i < resnum; ++i)
                        {
                            int w = 0, h = 0;
                            if (cam_.get_Resolution(i, out w, out h))
                                comboBox1.Items.Add(w.ToString() + "*" + h.ToString());
                        }
                        comboBox1.SelectedIndex = (int)eSize;

                        int width = 0, height = 0;
                        if (cam_.get_Size(out width, out height))
                        {
                            bmp_ = new Bitmap(width, height, PixelFormat.Format24bppRgb);
                            ev_ = new DelegateEvent(DelegateOnEvent);
                            if (!cam_.StartPullModeWithCallback(new Toupcam.DelegateEventCallback(DelegateOnEventCallback)))
                                MessageBox.Show("failed to start device");
                            else
                            {
                                bool autoexpo = true;
                                cam_.get_AutoExpoEnable(out autoexpo);
                                checkBox1.Checked = autoexpo;
                                trackBar1.Enabled = !checkBox1.Checked;
                            }
                        }
                    }
                }
            }
        }

        private void SnapClickedHandler(object sender, ToolStripItemClickedEventArgs e)
        {
            int k = button2.ContextMenuStrip.Items.IndexOf(e.ClickedItem);
            if (k >= 0)
                cam_.Snap((uint)k);
        }

        private void InitSnapContextMenuAndExpoTimeRange()
        {
            if (cam_ == null)
                return;

            uint nMin = 0, nMax = 0, nDef = 0;
            if (cam_.get_ExpTimeRange(out nMin, out nMax, out nDef))
                trackBar1.SetRange((int)nMin, (int)nMax);
            OnEventExposure();

            if (cam_.StillResolutionNumber <= 0)
                return;
            
            button2.ContextMenuStrip = new ContextMenuStrip();
            button2.ContextMenuStrip.ItemClicked += new ToolStripItemClickedEventHandler(this.SnapClickedHandler);

            if (cam_.StillResolutionNumber < cam_.ResolutionNumber)
            {
                uint eSize = 0;
                if (cam_.get_eSize(out eSize))
                {
                    if (0 == eSize)
                    {
                        StringBuilder sb = new StringBuilder();
                        int w = 0, h = 0;
                        cam_.get_Resolution(eSize, out w, out h);
                        sb.Append(w);
                        sb.Append(" * ");
                        sb.Append(h);
                        button2.ContextMenuStrip.Items.Add(sb.ToString());
                        return;
                    }
                }
            }

            for (uint i = 0; i < cam_.ResolutionNumber; ++i)
            {
                StringBuilder sb = new StringBuilder();
                int w = 0, h = 0;
                cam_.get_Resolution(i, out w, out h);
                sb.Append(w);
                sb.Append(" * ");
                sb.Append(h);
                button2.ContextMenuStrip.Items.Add(sb.ToString());
            }
        }

        private void OnSnap(object sender, EventArgs e)
        {
            if (cam_ != null)
            {
                if (cam_.StillResolutionNumber <= 0)
                {
                    if (bmp_ != null)
                    {
                        bmp_.Save("demowinformcs2.jpg");
                    }
                }
                else
                {
                    if (button2.ContextMenuStrip != null)
                        button2.ContextMenuStrip.Show(Cursor.Position);
                }
            }
        }

        private void OnClosing(object sender, FormClosingEventArgs e)
        {
            if (cam_ != null)
            {
                cam_.Close();
                cam_ = null;
            }
        }

        private void OnSelectResolution(object sender, EventArgs e)
        {
            if (cam_ != null)
            {
                uint eSize = 0;
                if (cam_.get_eSize(out eSize))
                {
                    if (eSize != comboBox1.SelectedIndex)
                    {
                        button2.ContextMenuStrip = null;

                        cam_.Stop();
                        cam_.put_eSize((uint)comboBox1.SelectedIndex);

                        InitSnapContextMenuAndExpoTimeRange();
                        OnEventTempTint();

                        int width = 0, height = 0;
                        if (cam_.get_Size(out width, out height))
                        {
                            bmp_ = new Bitmap(width, height, PixelFormat.Format24bppRgb);
                            ev_ = new DelegateEvent(DelegateOnEvent);
                            cam_.StartPullModeWithCallback(new Toupcam.DelegateEventCallback(DelegateOnEventCallback));
                        }
                    }
                }
            }
        }

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            if (cam_ != null)
                cam_.put_AutoExpoEnable(checkBox1.Checked);
            trackBar1.Enabled = !checkBox1.Checked;
        }

        private void OnExpoValueChange(object sender, EventArgs e)
        {
            if (!checkBox1.Checked)
            {
                if (cam_ != null)
                {
                    uint n = (uint)trackBar1.Value;
                    cam_.put_ExpoTime(n);
                    label1.Text = (n / 1000).ToString() + " ms";
                }
            }
        }

        private void Form_SizeChanged(object sender, EventArgs e)
        {
            pictureBox1.Width = ClientRectangle.Right - button1.Bounds.Right - 20;
            pictureBox1.Height = ClientRectangle.Height - 8;
        }

        private void OnEventTempTint()
        {
            if (cam_ != null)
            {
                int nTemp = 0, nTint = 0;
                if (cam_.get_TempTint(out nTemp, out nTint))
                {
                    label2.Text = nTemp.ToString();
                    label3.Text = nTint.ToString();
                    trackBar2.Value = nTemp;
                    trackBar3.Value = nTint;
                }
            }
        }

        private void OnWhiteBalanceOnePush(object sender, EventArgs e)
        {
            if (cam_ != null)
                cam_.AwbOnePush();
        }

        private void OnTempTintChanged(object sender, EventArgs e)
        {
            if (cam_ != null)
                cam_.put_TempTint(trackBar2.Value, trackBar3.Value);
            label2.Text = trackBar2.Value.ToString();
            label3.Text = trackBar3.Value.ToString();
        }
    }
}
