Imports System.Collections.Generic
Imports System.ComponentModel
Imports System.Data
Imports System.Drawing
Imports System.Drawing.Imaging
Imports System.Text
Imports System.Windows.Forms
Imports System.Runtime.InteropServices

Public Class Form1
    Private cam_ As Toupcam = Nothing
    Private bmp_ As Bitmap = Nothing
    Private MSG_CAMEVENT As UInteger = &H8001 ' WM_APP = 0x8000

    Private Sub OnEventError()
        If cam_ IsNot Nothing Then
            cam_.Close()
            cam_ = Nothing
        End If
        MessageBox.Show("Error")
    End Sub

    Private Sub OnEventDisconnected()
        If cam_ IsNot Nothing Then
            cam_.Close()
            cam_ = Nothing
        End If
        MessageBox.Show("The camera is disconnected, maybe has been pulled out.")
    End Sub

    Private Sub OnEventExposure()
        If cam_ IsNot Nothing Then
            Dim nTime As UInteger = 0
            If cam_.get_ExpoTime(nTime) Then
                trackBar1.Value = CInt(nTime)
                label1.Text = (nTime \ 1000).ToString() & " ms"
            End If
        End If
    End Sub

    Private Sub OnEventImage()
        If bmp_ IsNot Nothing Then
            Dim bmpdata As BitmapData = bmp_.LockBits(New Rectangle(0, 0, bmp_.Width, bmp_.Height), ImageLockMode.[WriteOnly], bmp_.PixelFormat)

            Dim info As New Toupcam.FrameInfoV2
            cam_.PullImageV2(bmpdata.Scan0, 24, info)

            bmp_.UnlockBits(bmpdata)

            pictureBox1.Image = bmp_
            pictureBox1.Invalidate()
        End If
    End Sub

    Private Sub OnEventStillImage()
        Dim info As New Toupcam.FrameInfoV2
        If cam_.PullStillImageV2(IntPtr.Zero, 24, info) Then ' peek the width and height 
            Dim sbmp As New Bitmap(CInt(info.width), CInt(info.height), PixelFormat.Format24bppRgb)

            Dim bmpdata As BitmapData = sbmp.LockBits(New Rectangle(0, 0, sbmp.Width, sbmp.Height), ImageLockMode.[WriteOnly], sbmp.PixelFormat)
            cam_.PullStillImageV2(bmpdata.Scan0, 24, info)
            sbmp.UnlockBits(bmpdata)

            sbmp.Save("demowinformvb.jpg")
        End If
    End Sub

    Public Sub New()
        InitializeComponent()
        pictureBox1.Width = ClientRectangle.Right - button1.Bounds.Right - 20
        pictureBox1.Height = ClientRectangle.Height - 8
    End Sub

    <System.Security.Permissions.PermissionSet(System.Security.Permissions.SecurityAction.Demand, Name:="FullTrust")> _
    Protected Overrides Sub WndProc(ByRef m As Message)
        If MSG_CAMEVENT = m.Msg Then
            Select Case m.WParam.ToInt32()
                Case Toupcam.eEVENT.EVENT_ERROR
                    OnEventError()
                    Exit Select
                Case Toupcam.eEVENT.EVENT_DISCONNECTED
                    OnEventDisconnected()
                    Exit Select
                Case Toupcam.eEVENT.EVENT_EXPOSURE
                    OnEventExposure()
                    Exit Select
                Case Toupcam.eEVENT.EVENT_IMAGE
                    OnEventImage()
                    Exit Select
                Case Toupcam.eEVENT.EVENT_STILLIMAGE
                    OnEventStillImage()
                    Exit Select
                Case Toupcam.eEVENT.EVENT_TEMPTINT
                    OnEventTempTint()
                    Exit Select
            End Select
            Return
        End If
        MyBase.WndProc(m)
    End Sub

    Private Sub SnapClickedHandler(sender As Object, e As ToolStripItemClickedEventArgs)
        Dim k As Integer = button2.ContextMenuStrip.Items.IndexOf(e.ClickedItem)
        If k >= 0 Then
            cam_.Snap(CUInt(k))
        End If
    End Sub

    Private Sub InitSnapContextMenuAndExpoTimeRange()
        If cam_ Is Nothing Then
            Return
        End If

        Dim nMin As UInteger = 0, nMax As UInteger = 0, nDef As UInteger = 0
        If cam_.get_ExpTimeRange(nMin, nMax, nDef) Then
            trackBar1.SetRange(CInt(nMin), CInt(nMax))
        End If
        OnEventExposure()

        If cam_.StillResolutionNumber <= 0 Then
            Return
        End If

        button2.ContextMenuStrip = New ContextMenuStrip()
        AddHandler button2.ContextMenuStrip.ItemClicked, AddressOf Me.SnapClickedHandler

        If cam_.StillResolutionNumber < cam_.ResolutionNumber Then
            Dim eSize As UInteger = 0
            If cam_.get_eSize(eSize) Then
                If 0 = eSize Then
                    Dim sb As New StringBuilder()
                    Dim w As Integer = 0, h As Integer = 0
                    cam_.get_Resolution(eSize, w, h)
                    sb.Append(w)
                    sb.Append(" * ")
                    sb.Append(h)
                    button2.ContextMenuStrip.Items.Add(sb.ToString())
                    Return
                End If
            End If
        End If

        For i As UInteger = 0 To cam_.ResolutionNumber - 1
            Dim sb As New StringBuilder()
            Dim w As Integer = 0, h As Integer = 0
            cam_.get_Resolution(i, w, h)
            sb.Append(w)
            sb.Append(" * ")
            sb.Append(h)
            button2.ContextMenuStrip.Items.Add(sb.ToString())
        Next
    End Sub

    Private Overloads Sub OnClosing(sender As Object, e As FormClosingEventArgs) Handles MyBase.FormClosing
        If cam_ IsNot Nothing Then
            cam_.Close()
            cam_ = Nothing
        End If
    End Sub

    Private Sub Form_SizeChanged(sender As Object, e As EventArgs) Handles MyBase.SizeChanged
        pictureBox1.Width = ClientRectangle.Right - button1.Bounds.Right - 20
        pictureBox1.Height = ClientRectangle.Height - 8
    End Sub

    Private Sub OnEventTempTint()
        If cam_ IsNot Nothing Then
            Dim nTemp As Integer = 0, nTint As Integer = 0
            If cam_.get_TempTint(nTemp, nTint) Then
                label2.Text = nTemp.ToString()
                label3.Text = nTint.ToString()
                trackBar2.Value = nTemp
                trackBar3.Value = nTint
            End If
        End If
    End Sub

    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        Me.button2.Enabled = False
        Me.button3.Enabled = False
        Me.trackBar1.Enabled = False
        Me.trackBar2.Enabled = False
        Me.trackBar3.Enabled = False
        Me.checkBox1.Enabled = False
        Me.comboBox1.Enabled = False
    End Sub

    Private Sub OnStart(sender As Object, e As EventArgs) Handles button1.Click
        If cam_ IsNot Nothing Then
            Return
        End If

        Dim arr As Toupcam.DeviceV2() = Toupcam.[EnumV2]()
        If arr.Length <= 0 Then
            MessageBox.Show("no device")
        Else
            cam_ = Toupcam.Open(arr(0).id)
            If cam_ IsNot Nothing Then
                checkBox1.Enabled = True
                trackBar1.Enabled = True
                trackBar2.Enabled = True
                trackBar3.Enabled = True
                comboBox1.Enabled = True
                button2.Enabled = True
                button3.Enabled = True
                button2.ContextMenuStrip = Nothing
                InitSnapContextMenuAndExpoTimeRange()

                trackBar2.SetRange(2000, 15000)
                trackBar3.SetRange(200, 2500)
                OnEventTempTint()

                Dim resnum As UInteger = cam_.ResolutionNumber
                Dim eSize As UInteger = 0
                If cam_.get_eSize(eSize) Then
                    For i As UInteger = 0 To resnum - 1
                        Dim w As Integer = 0, h As Integer = 0
                        If cam_.get_Resolution(i, w, h) Then
                            comboBox1.Items.Add(w.ToString() & "*" & h.ToString())
                        End If
                    Next
                    comboBox1.SelectedIndex = CInt(eSize)

                    Dim width As Integer = 0, height As Integer = 0
                    If cam_.get_Size(width, height) Then
                        bmp_ = New Bitmap(width, height, PixelFormat.Format24bppRgb)
                        If Not cam_.StartPullModeWithWndMsg(Me.Handle, MSG_CAMEVENT) Then
                            MessageBox.Show("failed to start device")
                        Else
                            Dim autoexpo As Boolean = True
                            cam_.get_AutoExpoEnable(autoexpo)
                            checkBox1.Checked = autoexpo
                            trackBar1.Enabled = Not checkBox1.Checked
                        End If
                    End If
                End If
            End If
        End If
    End Sub

    Private Sub OnSnap(sender As Object, e As EventArgs) Handles button2.Click
        If cam_ IsNot Nothing Then
            If cam_.StillResolutionNumber <= 0 Then
                If bmp_ IsNot Nothing Then
                    bmp_.Save("demowinformvb.jpg")
                End If
            Else
                If button2.ContextMenuStrip IsNot Nothing Then
                    button2.ContextMenuStrip.Show(Cursor.Position)
                End If
            End If
        End If
    End Sub

    Private Sub OnSelectResolution(sender As Object, e As EventArgs) Handles comboBox1.SelectedIndexChanged
        If cam_ IsNot Nothing Then
            Dim eSize As UInteger = 0
            If cam_.get_eSize(eSize) Then
                If eSize <> comboBox1.SelectedIndex Then
                    button2.ContextMenuStrip = Nothing

                    cam_.[Stop]()
                    cam_.put_eSize(CUInt(comboBox1.SelectedIndex))

                    InitSnapContextMenuAndExpoTimeRange()
                    OnEventTempTint()

                    Dim width As Integer = 0, height As Integer = 0
                    If cam_.get_Size(width, height) Then
                        bmp_ = New Bitmap(width, height, PixelFormat.Format24bppRgb)
                        cam_.StartPullModeWithWndMsg(Me.Handle, MSG_CAMEVENT)
                    End If
                End If
            End If
        End If
    End Sub

    Private Sub checkBox1_CheckedChanged(sender As Object, e As EventArgs) Handles checkBox1.CheckedChanged
        If cam_ IsNot Nothing Then
            cam_.put_AutoExpoEnable(checkBox1.Checked)
        End If
        trackBar1.Enabled = Not checkBox1.Checked
    End Sub

    Private Sub OnExpoValueChange(sender As Object, e As EventArgs) Handles trackBar1.ValueChanged
        If Not checkBox1.Checked Then
            If cam_ IsNot Nothing Then
                Dim n As UInteger = CUInt(trackBar1.Value)
                cam_.put_ExpoTime(n)
                label1.Text = (n \ 1000).ToString() & " ms"
            End If
        End If
    End Sub

    Private Sub OnWhiteBalanceOnePush(sender As Object, e As EventArgs) Handles button3.Click
        If cam_ IsNot Nothing Then
            cam_.AwbOnePush()
        End If
    End Sub

    Private Sub OnTempTintChanged(sender As Object, e As EventArgs) Handles trackBar2.ValueChanged, trackBar3.ValueChanged
        If cam_ IsNot Nothing Then
            cam_.put_TempTint(trackBar2.Value, trackBar3.Value)
        End If
        label2.Text = trackBar2.Value.ToString()
        label3.Text = trackBar3.Value.ToString()
    End Sub

End Class