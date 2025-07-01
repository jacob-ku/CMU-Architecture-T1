object Form1: TForm1
  Left = 0
  Top = 0
  Caption = 'ADS-B Viewer'
  ClientHeight = 1041
  ClientWidth = 1121
  Color = clBtnFace
  Constraints.MinHeight = 740
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -10
  Font.Name = 'Tahoma'
  Font.Style = [fsBold]
  Menu = MainMenu1
  Position = poScreenCenter
  WindowState = wsMaximized
  OnMouseWheel = FormMouseWheel
  TextHeight = 12
  object Label16: TLabel
    Left = 13
    Top = 67
    Width = 31
    Height = 13
    Caption = 'ICAO:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
  end
  object Label17: TLabel
    Left = 48
    Top = 67
    Width = 46
    Height = 13
    Caption = 'No Hook'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
  end
  object RightPanel: TPanel
    Left = 871
    Top = 0
    Width = 250
    Height = 1041
    Align = alRight
    Color = clWindow
    Constraints.MaxWidth = 250
    TabOrder = 0
    object Panel1: TPanel
      Left = 1
      Top = 893
      Width = 248
      Height = 147
      Align = alBottom
      TabOrder = 0
      object Label12: TLabel
        Left = 8
        Top = 8
        Width = 55
        Height = 12
        Caption = 'CPA TIME:'
      end
      object Label19: TLabel
        Left = 8
        Top = 26
        Width = 82
        Height = 12
        Caption = 'CPA DISTANCE:'
      end
      object CpaTimeValue: TLabel
        Left = 69
        Top = 8
        Width = 28
        Height = 12
        Caption = 'NONE'
      end
      object CpaDistanceValue: TLabel
        Left = 96
        Top = 26
        Width = 25
        Height = 12
        Caption = 'None'
      end
      object ZoomIn: TButton
        Left = 5
        Top = 110
        Width = 65
        Height = 18
        Caption = 'Zoom In'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = [fsBold]
        ParentFont = False
        TabOrder = 0
        OnClick = ZoomInClick
      end
      object ZoomOut: TButton
        Left = 160
        Top = 110
        Width = 65
        Height = 18
        Caption = 'Zoom Out'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = [fsBold]
        ParentFont = False
        TabOrder = 1
        OnClick = ZoomOutClick
      end
      object TimeToGoTrackBar: TTrackBar
        Left = 5
        Top = 71
        Width = 228
        Height = 33
        Max = 1800
        Min = 1
        Frequency = 100
        Position = 1
        TabOrder = 2
        OnChange = TimeToGoTrackBarChange
      end
      object TimeToGoCheckBox: TCheckBox
        Left = 8
        Top = 38
        Width = 105
        Height = 27
        Caption = 'Time-To-Go'
        Checked = True
        State = cbChecked
        TabOrder = 3
      end
      object TimeToGoText: TStaticText
        Left = 119
        Top = 43
        Width = 76
        Height = 16
        Caption = '00:00:00:000'
        TabOrder = 4
      end
    end
    object Panel3: TPanel
      Left = 1
      Top = 1
      Width = 248
      Height = 776
      Align = alTop
      Constraints.MinHeight = 70
      TabOrder = 1
      object Label15: TLabel
        Left = 72
        Top = 565
        Width = 98
        Height = 13
        Caption = 'Areas Of Interest'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = [fsBold]
        ParentFont = False
      end
      object DrawMap: TCheckBox
        Left = 5
        Top = 373
        Width = 96
        Height = 17
        Caption = 'Display Map'
        Checked = True
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = [fsBold]
        ParentFont = False
        State = cbChecked
        TabOrder = 0
      end
      object PurgeStale: TCheckBox
        Left = 5
        Top = 396
        Width = 96
        Height = 19
        Caption = 'Purge Stale'
        Checked = True
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = [fsBold]
        ParentFont = False
        State = cbChecked
        TabOrder = 1
      end
      object CSpinStaleTime: TCSpinEdit
        Left = 107
        Top = 373
        Width = 62
        Height = 21
        Increment = 5
        MaxValue = 1000
        MinValue = 5
        TabOrder = 2
        Value = 30
      end
      object PurgeButton: TButton
        Left = 186
        Top = 373
        Width = 51
        Height = 16
        Caption = 'Purge'
        TabOrder = 3
        OnClick = PurgeButtonClick
      end
      object AreaListView: TListView
        Left = 5
        Top = 584
        Width = 240
        Height = 82
        Columns = <
          item
            Caption = 'Area'
            Width = 170
          end
          item
            Caption = 'Color'
            Width = 40
          end>
        ReadOnly = True
        RowSelect = True
        TabOrder = 4
        ViewStyle = vsReport
        OnCustomDrawItem = AreaListViewCustomDrawItem
        OnSelectItem = AreaListViewSelectItem
      end
      object Insert: TButton
        Left = 5
        Top = 672
        Width = 36
        Height = 18
        Caption = 'Insert'
        TabOrder = 5
        OnClick = InsertClick
      end
      object Delete: TButton
        Left = 158
        Top = 672
        Width = 44
        Height = 18
        Caption = 'Delete'
        Enabled = False
        TabOrder = 6
        OnClick = DeleteClick
      end
      object Complete: TButton
        Left = 45
        Top = 672
        Width = 57
        Height = 18
        Caption = 'Complete'
        Enabled = False
        TabOrder = 7
        OnClick = CompleteClick
      end
      object Cancel: TButton
        Left = 107
        Top = 672
        Width = 41
        Height = 18
        Caption = 'Cancel'
        Enabled = False
        TabOrder = 8
        OnClick = CancelClick
      end
      object RawConnectButton: TButton
        Left = 5
        Top = 421
        Width = 104
        Height = 18
        Caption = 'Raw Connect'
        TabOrder = 9
        OnClick = RawConnectButtonClick
      end
      object RawIpAddress: TEdit
        Left = 115
        Top = 421
        Width = 110
        Height = 20
        TabOrder = 10
        Text = '172.20.2.97'
      end
      object CycleImages: TCheckBox
        Left = 107
        Top = 396
        Width = 96
        Height = 19
        Caption = 'Cycle Images'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = [fsBold]
        ParentFont = False
        TabOrder = 11
        OnClick = CycleImagesClick
      end
      object Panel4: TPanel
        Left = 5
        Top = 107
        Width = 240
        Height = 260
        BevelInner = bvLowered
        BevelKind = bkFlat
        TabOrder = 12
        object CLatLabel: TLabel
          Left = 33
          Top = 38
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object CLonLabel: TLabel
          Left = 36
          Top = 56
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
          Transparent = False
        end
        object SpdLabel: TLabel
          Left = 36
          Top = 74
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object HdgLabel: TLabel
          Left = 36
          Top = 92
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object AltLabel: TLabel
          Left = 36
          Top = 110
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object MsgCntLabel: TLabel
          Left = 65
          Top = 128
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object TrkLastUpdateTimeLabel: TLabel
          Left = 121
          Top = 146
          Width = 72
          Height = 13
          Caption = '00:00:00:000'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label14: TLabel
          Left = 5
          Top = 146
          Width = 109
          Height = 13
          Caption = 'LAST UPDATE TIME:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label13: TLabel
          Left = 5
          Top = 128
          Width = 52
          Height = 13
          Caption = 'MSG CNT:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label10: TLabel
          Left = 5
          Top = 110
          Width = 24
          Height = 13
          Caption = 'ALT:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label9: TLabel
          Left = 5
          Top = 92
          Width = 27
          Height = 13
          Caption = 'HDG:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label8: TLabel
          Left = 5
          Top = 74
          Width = 25
          Height = 13
          Caption = 'SPD:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label7: TLabel
          Left = 5
          Top = 56
          Width = 24
          Height = 13
          Caption = 'LON:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label6: TLabel
          Left = 5
          Top = 38
          Width = 24
          Height = 13
          Caption = 'LAT:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label18: TLabel
          Left = 117
          Top = 19
          Width = 55
          Height = 13
          Caption = 'FLIGHT #:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object FlightNumLabel: TLabel
          Left = 178
          Top = 19
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object ICAOLabel: TLabel
          Left = 42
          Top = 19
          Width = 46
          Height = 13
          Caption = 'No Hook'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label5: TLabel
          Left = 5
          Top = 19
          Width = 31
          Height = 13
          Caption = 'ICAO:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label4: TLabel
          Left = 75
          Top = 0
          Width = 85
          Height = 16
          Caption = 'Close Control'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'Tahoma'
          Font.Style = [fsBold, fsUnderline]
          ParentFont = False
        end
        object Label20: TLabel
          Left = 5
          Top = 164
          Width = 85
          Height = 13
          Caption = 'Registration #:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object RegNumLabel: TLabel
          Left = 96
          Top = 164
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label24: TLabel
          Left = 5
          Top = 182
          Width = 57
          Height = 13
          Caption = 'Operator :'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object OperatorLabel: TLabel
          Left = 68
          Top = 182
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label26: TLabel
          Left = 5
          Top = 200
          Width = 51
          Height = 13
          Caption = 'Country :'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object CountryLabel: TLabel
          Left = 62
          Top = 200
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object TypeLabel: TLabel
          Left = 45
          Top = 218
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label25: TLabel
          Left = 5
          Top = 218
          Width = 34
          Height = 13
          Caption = 'Type :'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object RouteLabel: TLabel
          Left = 51
          Top = 236
          Width = 21
          Height = 13
          Caption = 'N/A'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label27: TLabel
          Left = 5
          Top = 236
          Width = 40
          Height = 13
          Caption = 'Route :'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
      end
      object Panel5: TPanel
        Left = 5
        Top = 2
        Width = 240
        Height = 99
        BevelInner = bvLowered
        BevelKind = bkFlat
        TabOrder = 13
        object Lon: TLabel
          Left = 31
          Top = 40
          Width = 7
          Height = 13
          Caption = '0'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label3: TLabel
          Left = 5
          Top = 40
          Width = 23
          Height = 13
          Caption = 'Lon:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Lat: TLabel
          Left = 31
          Top = 21
          Width = 7
          Height = 13
          Caption = '0'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label2: TLabel
          Left = 5
          Top = 21
          Width = 21
          Height = 13
          Caption = 'Lat:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object SystemTimeLabel: TLabel
          Left = 5
          Top = 4
          Width = 74
          Height = 13
          Caption = 'System Time'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object ViewableAircraftCountLabel: TLabel
          Left = 157
          Top = 59
          Width = 7
          Height = 12
          Caption = '0'
        end
        object AircraftCountLabel: TLabel
          Left = 66
          Top = 58
          Width = 7
          Height = 13
          Caption = '0'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label11: TLabel
          Left = 101
          Top = 58
          Width = 53
          Height = 13
          Caption = 'Viewable:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label1: TLabel
          Left = 5
          Top = 58
          Width = 58
          Height = 13
          Caption = '# Aircraft:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object UnregisteredCount: TLabel
          Left = 5
          Top = 77
          Width = 87
          Height = 13
          Caption = 'Unregistered: 0'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object SystemTime: TStaticText
          Left = 85
          Top = 4
          Width = 76
          Height = 17
          Caption = '00:00:00:000'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
          TabOrder = 0
        end
      end
      object RawPlaybackButton: TButton
        Left = 115
        Top = 445
        Width = 96
        Height = 17
        Caption = 'Raw Playback'
        TabOrder = 14
        OnClick = RawPlaybackButtonClick
      end
      object RawRecordButton: TButton
        Left = 5
        Top = 445
        Width = 104
        Height = 17
        Caption = 'Raw Record'
        TabOrder = 15
        OnClick = RawRecordButtonClick
      end
      object SBSConnectButton: TButton
        Left = 5
        Top = 468
        Width = 104
        Height = 20
        Caption = 'SBS Connect'
        TabOrder = 16
        OnClick = SBSConnectButtonClick
      end
      object SBSIpAddress: TEdit
        Left = 115
        Top = 468
        Width = 117
        Height = 20
        TabOrder = 17
        Text = 'data.adsbhub.org'
      end
      object SBSRecordButton: TButton
        Left = 5
        Top = 494
        Width = 76
        Height = 17
        Caption = 'SBS Record'
        TabOrder = 18
        OnClick = SBSRecordButtonClick
      end
      object SBSPlaybackButton: TButton
        Left = 87
        Top = 494
        Width = 82
        Height = 17
        Caption = 'SBS Playback'
        TabOrder = 19
        OnClick = SBSPlaybackButtonClick
      end
      object SBSPlaybackSpeedComboBox: TComboBox
        Left = 175
        Top = 494
        Width = 58
        Height = 20
        TabOrder = 20
        Text = '1.0'
        Items.Strings = (
          '1.0'
          '2.0'
          '3.0')
      end
      object SearchAircraft: TButton
        Left = 5
        Top = 517
        Width = 103
        Height = 20
        Caption = 'Search Aircraft'
        TabOrder = 21
        OnClick = SearchAircraftClick
      end
      object AircraftNumber: TEdit
        Left = 115
        Top = 517
        Width = 118
        Height = 20
        TabOrder = 22
        TextHint = 'Enter Aircraft Number'
        OnChange = AircraftNumberChange
      end
    end
    object Panel2: TPanel
      Left = 1
      Top = 777
      Width = 248
      Height = 116
      Align = alClient
      TabOrder = 2
      object LabelErrorMessage: TLabel
        Left = 13
        Top = 83
        Width = 150
        Height = 13
        Caption = 'Error Message Placeholder'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = [fsBold]
        ParentFont = False
      end
      object MapComboBox: TComboBox
        Left = 5
        Top = 6
        Width = 108
        Height = 20
        Style = csDropDownList
        TabOrder = 0
        OnChange = MapComboBoxChange
        Items.Strings = (
          'Google Maps'
          'Google Street View'
          'Google Terrain with Labels'
          'VFR'
          'IFS Low'
          'IFS High'
          'Open Street')
      end
      object CheckBoxUpdateMapTiles: TCheckBox
        Left = 119
        Top = 7
        Width = 150
        Height = 17
        Caption = 'Update map tiles'
        TabOrder = 1
        OnClick = CheckBoxUpdateMapTilesClick
      end
      object BigQueryCheckBox: TCheckBox
        Left = 5
        Top = 37
        Width = 124
        Height = 17
        Caption = 'BigQuery Upload'
        TabOrder = 2
        OnClick = BigQueryCheckBoxClick
      end
      object HideUnregisteredCheckBox: TCheckBox
        Left = 5
        Top = 60
        Width = 120
        Height = 17
        Caption = 'Hide unregistered'
        TabOrder = 3
        OnClick = HideUnregisteredCheckBoxClick
      end
    end
  end
  object ObjectDisplay: TOpenGLPanel
    Left = 0
    Top = 0
    Width = 871
    Height = 1041
    Cursor = crCross
    Align = alClient
    TabOrder = 1
    PFDFlags = [f_PFD_DRAW_TO_WINDOW, f_PFD_SUPPORT_OPENGL, f_PFD_DOUBLEBUFFER]
    Font3D_Type.Charset = ANSI_CHARSET
    Font3D_Type.Color = clWindowText
    Font3D_Type.Height = -9
    Font3D_Type.Name = 'Arial'
    Font3D_Type.Style = []
    Font3D_Extrustion = 0.100000001490116100
    Font2D_Enabled = True
    Font2D_Type.Charset = ANSI_CHARSET
    Font2D_Type.Color = clWindowText
    Font2D_Type.Height = -27
    Font2D_Type.Name = 'Arial'
    Font2D_Type.Style = [fsBold]
    Font2D_AdditionalType.Charset = ANSI_CHARSET
    Font2D_AdditionalType.Color = clWindowText
    Font2D_AdditionalType.Height = -15
    Font2D_AdditionalType.Name = 'Arial'
    Font2D_AdditionalType.Style = [fsBold]
    OnMouseDown = ObjectDisplayMouseDown
    OnMouseMove = ObjectDisplayMouseMove
    OnMouseUp = ObjectDisplayMouseUp
    OnInit = ObjectDisplayInit
    OnResize = ObjectDisplayResize
    OnPaint = ObjectDisplayPaint
  end
  object MainMenu1: TMainMenu
    Left = 24
    object File1: TMenuItem
      Caption = 'File'
      object UseSBSRemote: TMenuItem
        Caption = 'SBS Hub'
        OnClick = UseSBSRemoteClick
      end
      object UseSBSLocal: TMenuItem
        Caption = 'SBS Local'
        OnClick = UseSBSLocalClick
      end
      object LoadARTCCBoundaries1: TMenuItem
        Caption = 'Load ARTCC Boundaries'
        OnClick = LoadARTCCBoundaries1Click
      end
      object UseRawRouter: TMenuItem
        Caption = 'ADS-B Router'
        OnClick = UseRawRouterClick
      end
      object UseRawCmuSecure: TMenuItem
        Caption = 'ADS-B CMU-SECURE'
        OnClick = UseRawCmuSecureClick
      end
      object UseRawHyattClick: TMenuItem
        Caption = 'ADS-B Hyatt'
        OnClick = UseRawHyattClick
      end
      object Exit1: TMenuItem
        Caption = 'Exit'
        OnClick = Exit1Click
      end
    end
  end
  object Timer1: TTimer
    Interval = 500
    OnTimer = Timer1Timer
    Left = 24
    Top = 64
  end
  object Timer2: TTimer
    Interval = 5000
    OnTimer = Timer2Timer
    Left = 16
    Top = 120
  end
  object IdTCPClientRaw: TIdTCPClient
    OnDisconnected = IdTCPClientRawDisconnected
    OnConnected = IdTCPClientRawConnected
    ConnectTimeout = 0
    Port = 0
    ReadTimeout = -1
    ReuseSocket = rsTrue
    Left = 16
    Top = 192
  end
  object RecordRawSaveDialog: TSaveDialog
    DefaultExt = 'raw'
    Filter = 'raw|*.raw'
    Left = 16
    Top = 256
  end
  object PlaybackRawDialog: TOpenDialog
    DefaultExt = 'raw'
    Filter = 'raw|*.raw'
    Left = 16
    Top = 320
  end
  object IdTCPClientSBS: TIdTCPClient
    OnDisconnected = IdTCPClientSBSDisconnected
    OnConnected = IdTCPClientSBSConnected
    ConnectTimeout = 0
    Port = 0
    ReadTimeout = -1
    ReuseSocket = rsTrue
    Left = 16
    Top = 392
  end
  object RecordSBSSaveDialog: TSaveDialog
    DefaultExt = 'sbs'
    Filter = 'sbs|*.sbs'
    Left = 16
    Top = 456
  end
  object PlaybackSBSDialog: TOpenDialog
    DefaultExt = 'sbs'
    Filter = 'sbs|*.sbs'
    Left = 16
    Top = 512
  end
end
