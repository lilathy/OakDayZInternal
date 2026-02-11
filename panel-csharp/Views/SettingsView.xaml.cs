using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;

namespace OakPanel.Views
{
    public partial class SettingsView : UserControl, INotifyPropertyChanged
    {
        private readonly SharedMemoryManager _sharedMemory;
        private int _menuKey;
        private bool _streamProof;
        private bool _isCapturingKey = false;
        private DispatcherTimer? _updateTimer;
        
        private string _coordX = "0.00";
        private string _coordY = "0.00";
        private string _coordZ = "0.00";
        private bool _dllAttached;
        private uint _targetPid;

        public int MenuKey
        {
            get => _menuKey;
            set { if (_menuKey != value) { _menuKey = value; OnPropertyChanged(); OnPropertyChanged(nameof(MenuKeyDisplay)); UpdateConfig(); } }
        }

        public string MenuKeyDisplay => _isCapturingKey ? "Press a key..." : GetKeyName(_menuKey);

        public bool StreamProof
        {
            get => _streamProof;
            set { if (_streamProof != value) { _streamProof = value; OnPropertyChanged(); UpdateConfig(); } }
        }

        public string CoordX
        {
            get => _coordX;
            set { if (_coordX != value) { _coordX = value; OnPropertyChanged(); } }
        }

        public string CoordY
        {
            get => _coordY;
            set { if (_coordY != value) { _coordY = value; OnPropertyChanged(); } }
        }

        public string CoordZ
        {
            get => _coordZ;
            set { if (_coordZ != value) { _coordZ = value; OnPropertyChanged(); } }
        }

        public bool DllAttached
        {
            get => _dllAttached;
            set { if (_dllAttached != value) { _dllAttached = value; OnPropertyChanged(); OnPropertyChanged(nameof(DllStatusDisplay)); UpdateStatusIndicator(); } }
        }

        public string DllStatusDisplay => _dllAttached ? "Connected" : "Disconnected";

        public uint TargetPid
        {
            get => _targetPid;
            set { if (_targetPid != value) { _targetPid = value; OnPropertyChanged(); OnPropertyChanged(nameof(TargetPidDisplay)); } }
        }

        public string TargetPidDisplay => _targetPid > 0 ? $"PID: {_targetPid}" : "Not attached";

        public SettingsView(SharedMemoryManager sharedMemory)
        {
            InitializeComponent();
            _sharedMemory = sharedMemory;
            DataContext = this;
            LoadSettings();
            SetupUpdateTimer();
        }

        private void LoadSettings()
        {
            var config = _sharedMemory.Config;
            _menuKey = config.MenuKey;
            _streamProof = config.StreamProof;
            _dllAttached = config.DllAttached;
            _targetPid = config.TargetPid;
            _coordX = config.PlayerX.ToString("F2");
            _coordY = config.PlayerY.ToString("F2");
            _coordZ = config.PlayerZ.ToString("F2");
            UpdateStatusIndicator();
        }

        private void SetupUpdateTimer()
        {
            _updateTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(250)
            };
            _updateTimer.Tick += (s, e) =>
            {
                _sharedMemory.ReadConfig();
                var config = _sharedMemory.Config;
                
                CoordX = config.PlayerX.ToString("F2");
                CoordY = config.PlayerY.ToString("F2");
                CoordZ = config.PlayerZ.ToString("F2");
                DllAttached = config.DllAttached;
                TargetPid = config.TargetPid;
            };
            _updateTimer.Start();
        }

        private void UpdateStatusIndicator()
        {
            try
            {
                DllStatusDot.Fill = _dllAttached 
                    ? (Brush)FindResource("SuccessBrush") 
                    : (Brush)FindResource("DangerBrush");
            }
            catch
            {
                
            }
        }

        private void UpdateConfig()
        {
            _sharedMemory.UpdateConfig(cfg =>
            {
                cfg.MenuKey = _menuKey;
                cfg.StreamProof = _streamProof;
                return cfg;
            });
        }

        private void MenuKey_Click(object sender, RoutedEventArgs e)
        {
            _isCapturingKey = true;
            OnPropertyChanged(nameof(MenuKeyDisplay));
            MenuKeyButton.Focus();
            
            KeyEventHandler? keyHandler = null;
            
            keyHandler = (s, args) =>
            {
                MenuKey = KeyInterop.VirtualKeyFromKey(args.Key);
                _isCapturingKey = false;
                OnPropertyChanged(nameof(MenuKeyDisplay));
                MenuKeyButton.KeyDown -= keyHandler;
                args.Handled = true;
            };
            
            MenuKeyButton.KeyDown += keyHandler;
        }

        private void ResetDefaults_Click(object sender, RoutedEventArgs e)
        {
            var result = MessageBox.Show(
                "reset everything to default?",
                "reset",
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);
            
            if (result == MessageBoxResult.Yes)
            {
                _sharedMemory.ResetToDefaults();
                LoadSettings();
                OnPropertyChanged(string.Empty); 
                
                MessageBox.Show(
                    "reset done",
                    "ok",
                    MessageBoxButton.OK,
                    MessageBoxImage.Information);
            }
        }

        private void CopyCoords_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var config = _sharedMemory.Config;
                string coords = $"X: {config.PlayerX:F2}, Y: {config.PlayerY:F2}, Z: {config.PlayerZ:F2}";
                Clipboard.SetText(coords);
                
                
                if (sender is Button btn)
                {
                    string originalText = btn.Content.ToString() ?? "Copy Coordinates";
                    btn.Content = "Copied!";
                    
                    var timer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1.5) };
                    timer.Tick += (s, args) =>
                    {
                        btn.Content = originalText;
                        timer.Stop();
                    };
                    timer.Start();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"copy failed: {ex.Message}", "error", 
                    MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private static string GetKeyName(int vk)
        {
            return vk switch
            {
                0x01 => "Mouse 1",
                0x02 => "Mouse 2",
                0x04 => "Mouse 3",
                0x05 => "Mouse 4",
                0x06 => "Mouse 5",
                0x08 => "Backspace",
                0x09 => "Tab",
                0x0D => "Enter",
                0x10 => "Shift",
                0x11 => "Ctrl",
                0x12 => "Alt",
                0x14 => "Caps Lock",
                0x1B => "Escape",
                0x20 => "Space",
                0x2D => "Insert",
                0x2E => "Delete",
                0x24 => "Home",
                0x23 => "End",
                0x21 => "Page Up",
                0x22 => "Page Down",
                0x25 => "Left",
                0x26 => "Up",
                0x27 => "Right",
                0x28 => "Down",
                >= 0x30 and <= 0x39 => ((char)vk).ToString(),
                >= 0x41 and <= 0x5A => ((char)vk).ToString(),
                >= 0x60 and <= 0x69 => $"Num {vk - 0x60}",
                >= 0x70 and <= 0x7B => $"F{vk - 0x6F}",
                0xC0 => "`",
                0xBD => "-",
                0xBB => "=",
                0xDB => "[",
                0xDD => "]",
                0xDC => "\\",
                0xBA => ";",
                0xDE => "'",
                0xBC => ",",
                0xBE => ".",
                0xBF => "/",
                _ => $"Key {vk}"
            };
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }
}
