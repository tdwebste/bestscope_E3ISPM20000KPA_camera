using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace demowinformcs3
{
    public partial class Form1 : Form
    {
        private delegate void DelegateOnImage(int[] roundrobin);
        private delegate void DelegateOnEvent(Toupcam.eEVENT[] nEvent);
        private Toupcam cam_ = null;
        private Bitmap bmp1_ = null;
        private Bitmap bmp2_ = null;
        private int roundrobin_ = 2;
        private Object locker_ = new Object();
        private DelegateOnEvent evevent_ = null;
        private DelegateOnImage evimage_ = null;
        
        private void OnEvent(Toupcam.eEVENT[] nEvent)
        {
            OnEvent(nEvent[0]);
        }

        private void OnEvent(Toupcam.eEVENT nEvent)
        {
            if (cam_ != null)
            {
                switch (nEvent)
                {
                    case Toupcam.eEVENT.EVENT_TEMPTINT:
                        int nTemp = 0, nTint = 0;
                        if (cam_.get_TempTint(out nTemp, out nTint))
                        {
                            label2.Text = nTemp.ToString();
                            label3.Text = nTint.ToString();
                            trackBar2.Value = nTemp;
                            trackBar3.Value = nTint;
                        }
                        break;
                    case Toupcam.eEVENT.EVENT_EXPOSURE:
                        uint nTime = 0;
                        if (cam_.get_ExpoTime(out nTime))
                        {
                            trackBar1.Value = (int)nTime;
                            label1.Text = (nTime / 1000).ToString() + " ms";
                        }
                        break;
                    case Toupcam.eEVENT.EVENT_ERROR:
                        cam_.Close();
                        cam_ = null;
                        MessageBox.Show("Error");
                        break;
                    default:
                        break;
                }
            }
        }

        private void OnEventImage(int[] roundrobin)
        {
            lock (locker_)
            {
                if (roundrobin[0] == 1)
                    pictureBox1.Image = bmp1_;
                else
                    pictureBox1.Image = bmp2_;
                roundrobin_ = roundrobin[0];
            }
            pictureBox1.Invalidate();
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

        /* these delegates are called by internal thread of toupcam.dll which is NOT the same of UI thread.
          * Why we use BeginInvoke, Please see:
          * http://msdn.microsoft.com/en-us/magazine/cc300429.aspx
          * http://msdn.microsoft.com/en-us/magazine/cc188732.aspx
          * http://stackoverflow.com/questions/1364116/avoiding-the-woes-of-invoke-begininvoke-in-cross-thread-winform-event-handling
        */
        
        void DelegateOnEventCallback(Toupcam.eEVENT nEvent)
        {
            BeginInvoke(evevent_, new Toupcam.eEVENT[] { nEvent });
        }

        void DelegateOnDataCallback(IntPtr pData, ref Toupcam.FrameInfoV2 info, bool bSnap)
        {
            if (pData == IntPtr.Zero)
            {
                /* something error */
                BeginInvoke(evevent_, new Toupcam.eEVENT[] { Toupcam.eEVENT.EVENT_ERROR });
            }
            else if (bSnap)
            {
                Bitmap sbmp = new Bitmap((int)info.width, (int)info.height, PixelFormat.Format24bppRgb);

                BitmapData bmpdata = sbmp.LockBits(new Rectangle(0, 0, sbmp.Width, sbmp.Height), ImageLockMode.WriteOnly, sbmp.PixelFormat);
                Toupcam.CopyMemory(bmpdata.Scan0, pData, new IntPtr((info.width * 24 + 31) / 32 * 4 * info.height));
                sbmp.UnlockBits(bmpdata);

                sbmp.Save("demowinformcs3.jpg");
            }
            else
            {
                int[] roundrobin = new int[] { 1 };
                lock (locker_)
                {
                	/* use two round robin bitmap objects to hold the image data */
                    if (1 == roundrobin_)
                    {
                        BitmapData bmpdata = bmp2_.LockBits(new Rectangle(0, 0, (int)info.width, (int)info.height), ImageLockMode.WriteOnly, bmp2_.PixelFormat);
                        Toupcam.CopyMemory(bmpdata.Scan0, pData, new IntPtr((info.width * 24 + 31) / 32 * 4 * info.height));
                        bmp2_.UnlockBits(bmpdata);
                        roundrobin[0] = 2;
                    }
                    else
                    {
                        BitmapData bmpdata = bmp1_.LockBits(new Rectangle(0, 0, (int)info.width, (int)info.height), ImageLockMode.WriteOnly, bmp1_.PixelFormat);
                        Toupcam.CopyMemory(bmpdata.Scan0, pData, new IntPtr((info.width * 24 + 31) / 32 * 4 * info.height));
                        bmp1_.UnlockBits(bmpdata);
                    }
                }
                BeginInvoke(evimage_, roundrobin);
            }
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
                    OnEvent(Toupcam.eEVENT.EVENT_TEMPTINT);

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
                            bmp1_ = new Bitmap(width, height, PixelFormat.Format24bppRgb);
                            bmp2_ = new Bitmap(width, height, PixelFormat.Format24bppRgb);
                            evimage_ = new DelegateOnImage(OnEventImage);
                            evevent_ = new DelegateOnEvent(OnEvent);
                            if (!cam_.StartPushModeV3(new Toupcam.DelegateDataCallbackV3(DelegateOnDataCallback), new Toupcam.DelegateEventCallback(DelegateOnEventCallback)))
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
            OnEvent(Toupcam.eEVENT.EVENT_EXPOSURE);

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
                    lock (locker_)
                    {
                        if (pictureBox1.Image != null)
                            pictureBox1.Image.Save("demowinformcs3.jpg");
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
                        OnEvent(Toupcam.eEVENT.EVENT_TEMPTINT);

                        int width = 0, height = 0;
                        if (cam_.get_Size(out width, out height))
                        {
                            bmp1_ = new Bitmap(width, height, PixelFormat.Format24bppRgb);
                            bmp2_ = new Bitmap(width, height, PixelFormat.Format24bppRgb);
                            evimage_ = new DelegateOnImage(OnEventImage);
                            evevent_ = new DelegateOnEvent(OnEvent);
                            cam_.StartPushModeV3(new Toupcam.DelegateDataCallbackV3(DelegateOnDataCallback), new Toupcam.DelegateEventCallback(DelegateOnEventCallback));
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
