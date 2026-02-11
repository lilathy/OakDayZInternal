using System;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Threading;
using OakPanel.Views;

namespace OakPanel
{
    public partial class MainWindow : Window
    {
        private const int WM_HOTKEY = 0x0312;
        private const uint MOD_NOREPEAT = 0x4000;
        private const int MENU_TOGGLE_HOTKEY_ID = 1;

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool RegisterHotKey(IntPtr hWnd, int id, uint fsModifiers, uint vk);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool UnregisterHotKey(IntPtr hWnd, int id);

        private SharedMemoryManager? _sharedMemory;
        private int _currentTab = 0;
        private DispatcherTimer? _statusTimer;
        private Button[] _tabButtons = Array.Empty<Button>();

        private HwndSource? _hotkeyHwndSource;
        private int _registeredMenuKey = -1;
        private DispatcherTimer? _hotkeyRefreshTimer;

        private double _smoothScrollTarget;
        private bool _smoothScrollRenderingSubscribed;
        private bool _smoothScrollFromUs;
        private const double SmoothScrollLerp = 0.16;
        private const double SmoothScrollWheelFactor = 0.45;
        private const double SmoothScrollStopThreshold = 0.5;

        public MainWindow()
        {
            try
            {
                InitializeComponent();
                SetupWindow();
                InitializeSharedMemory();
                SetupStatusTimer();
                _tabButtons = new[] { VisualsTab, SettingsTab };
                LoadTab(0);
                SourceInitialized += MainWindow_SourceInitialized;
            }
            catch (Exception ex)
            {
                MessageBox.Show($"panel init failed:\n\n{ex.Message}\n\n{ex.StackTrace}", 
                    "init error", MessageBoxButton.OK, MessageBoxImage.Error);
                Application.Current.Shutdown();
            }
        }

        private void SetupWindow()
        {
            Left = (SystemParameters.PrimaryScreenWidth - Width) / 2;
            Top = (SystemParameters.PrimaryScreenHeight - Height) / 2;
        }

        private void MainScrollViewer_Loaded(object sender, RoutedEventArgs e)
        {
            if (sender is not ScrollViewer scrollViewer) return;
            _smoothScrollTarget = scrollViewer.VerticalOffset;

            scrollViewer.ApplyTemplate();
            var scrollBar = scrollViewer.Template?.FindName("PART_VerticalScrollBar", scrollViewer) as ScrollBar;
            if (scrollBar == null) return;

            void ApplyFlip()
            {
                double h = scrollBar.ActualHeight;
                if (h <= 0) return;
                scrollBar.LayoutTransform = new ScaleTransform(1.0, -1.0) { CenterY = h / 2.0 };
            }

            ApplyFlip();
            scrollBar.SizeChanged += (_, _) => ApplyFlip();
        }

        private void MainScrollViewer_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (sender is not ScrollViewer sv) return;
            double max = Math.Max(0, sv.ScrollableHeight);
            if (max <= 0) return;

            _smoothScrollTarget -= e.Delta * SmoothScrollWheelFactor;
            _smoothScrollTarget = Math.Clamp(_smoothScrollTarget, 0, max);
            e.Handled = true;

            if (!_smoothScrollRenderingSubscribed)
            {
                _smoothScrollRenderingSubscribed = true;
                CompositionTarget.Rendering += SmoothScrollRendering;
            }
        }

        private void MainScrollViewer_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            if (_smoothScrollFromUs)
            {
                _smoothScrollFromUs = false;
                return;
            }
            if (sender is ScrollViewer sv)
                _smoothScrollTarget = sv.VerticalOffset;
        }

        private void SmoothScrollRendering(object? sender, EventArgs e)
        {
            var sv = MainScrollViewer;
            if (sv == null) return;

            double current = sv.VerticalOffset;
            double diff = _smoothScrollTarget - current;

            if (Math.Abs(diff) < SmoothScrollStopThreshold)
            {
                _smoothScrollFromUs = true;
                sv.ScrollToVerticalOffset(_smoothScrollTarget);
                _smoothScrollRenderingSubscribed = false;
                CompositionTarget.Rendering -= SmoothScrollRendering;
                return;
            }

            double newOffset = current + diff * SmoothScrollLerp;
            _smoothScrollFromUs = true;
            sv.ScrollToVerticalOffset(newOffset);
        }

        private void InitializeSharedMemory()
        {
            try
            {
                _sharedMemory = new SharedMemoryManager();
                if (!_sharedMemory.Initialize())
                {
                    System.Threading.Thread.Sleep(100);
                    if (!_sharedMemory.Initialize())
                    {
                        var result = MessageBox.Show(
                            "shared memory didnt init. panel might not talk to the dll.\n\ncontinue anyway?",
                            "shared memory", 
                            MessageBoxButton.YesNo, 
                            MessageBoxImage.Warning);
                        
                        if (result == MessageBoxResult.No)
                        {
                            Application.Current.Shutdown();
                            return;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                var result = MessageBox.Show(
                    $"shared memory error:\n\n{ex.Message}\n\ncontinue anyway?",
                    "shared memory",
                    MessageBoxButton.YesNo,
                    MessageBoxImage.Warning);
                
                if (result == MessageBoxResult.No)
                {
                    Application.Current.Shutdown();
                }
            }
        }

        private void SetupStatusTimer()
        {
            _statusTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(250)
            };
            _statusTimer.Tick += StatusTimer_Tick;
            _statusTimer.Start();
        }

        private void StatusTimer_Tick(object? sender, EventArgs e)
        {
            if (_sharedMemory == null) return;

            try
            {
                _sharedMemory.ReadConfig();
                var config = _sharedMemory.Config;

                
                CoordX.Text = config.PlayerX.ToString("F1");
                CoordY.Text = config.PlayerY.ToString("F1");
                CoordZ.Text = config.PlayerZ.ToString("F1");
            }
            catch
            {
                
            }
        }

        private void LoadTab(int tabIndex)
        {
            try
            {
                _currentTab = tabIndex;
                UpdateTabButtons();
                AnimateTabIndicatorTo(tabIndex);
                
                if (_sharedMemory == null)
                {
                    ContentArea.Content = new TextBlock 
                    { 
                        Text = "shared memory not init. restart the app.",
                        Foreground = new SolidColorBrush(Colors.Red),
                        Margin = new Thickness(20)
                    };
                    return;
                }

                
                var fadeOut = new DoubleAnimation(1, 0, TimeSpan.FromMilliseconds(100));
                fadeOut.Completed += (s, e) =>
                {
                    ContentArea.Content = tabIndex switch
                    {
                        0 => new VisualsView(_sharedMemory),
                        1 => new SettingsView(_sharedMemory),
                        _ => new VisualsView(_sharedMemory)
                    };

                    var fadeIn = new DoubleAnimation(0, 1, TimeSpan.FromMilliseconds(200));
                    ContentArea.BeginAnimation(OpacityProperty, fadeIn);
                };
                ContentArea.BeginAnimation(OpacityProperty, fadeOut);
            }
            catch (Exception ex)
            {
                ContentArea.Content = new TextBlock 
                { 
                    Text = $"Error loading tab:\n\n{ex.Message}",
                    Foreground = new SolidColorBrush(Colors.Red),
                    Margin = new Thickness(20),
                    TextWrapping = TextWrapping.Wrap
                };
            }
        }

        private void TabButton_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && int.TryParse(btn.Tag?.ToString(), out int tabIndex))
            {
                if (tabIndex != _currentTab)
                {
                    LoadTab(tabIndex);
                }
            }
        }

        private void UpdateTabButtons()
        {
            var activeStyle = (Style)FindResource("TabButtonActiveStyle");
            var normalStyle = (Style)FindResource("TabButtonStyle");

            for (int i = 0; i < _tabButtons.Length; i++)
            {
                _tabButtons[i].Style = i == _currentTab ? activeStyle : normalStyle;
            }
        }

        private void AnimateTabIndicatorTo(int tabIndex)
        {
            if (TabContainerGrid == null || TabIndicatorTransform == null || TabIndicator == null) return;
            Button? target = tabIndex == 0 ? VisualsTab : SettingsTab;
            if (target == null) return;

            Dispatcher.BeginInvoke(DispatcherPriority.Loaded, () =>
            {
                try
                {
                    Point p = target.TranslatePoint(new Point(0, 0), TabContainerGrid);
                    double buttonCenterY = p.Y + target.ActualHeight / 2;
                    double indicatorHalfH = TabIndicator.ActualHeight > 0 ? TabIndicator.ActualHeight / 2 : 22;
                    double targetY = buttonCenterY - indicatorHalfH;
                    double from = TabIndicatorTransform.Y;
                    var anim = new DoubleAnimation(from, targetY, TimeSpan.FromMilliseconds(220))
                    {
                        EasingFunction = new CubicEase { EasingMode = EasingMode.EaseOut }
                    };
                    TabIndicatorTransform.BeginAnimation(TranslateTransform.YProperty, anim);
                }
                catch { }
            });
        }

        private void TitleBar_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ClickCount == 2)
            {
                MaximizeButton_Click(sender, e);
            }
            else if (e.LeftButton == MouseButtonState.Pressed)
            {
                DragMove();
            }
        }

        private void CloseButton_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void CloseButton_MouseEnter(object sender, MouseEventArgs e)
        {
            if (sender is Button btn)
            {
                btn.Background = (Brush)FindResource("DangerBrush");
                btn.Foreground = Brushes.White;
            }
        }

        private void CloseButton_MouseLeave(object sender, MouseEventArgs e)
        {
            if (sender is Button btn)
            {
                btn.Background = Brushes.Transparent;
                btn.Foreground = (Brush)FindResource("TextDimBrush");
            }
        }

        private void MinimizeButton_Click(object sender, RoutedEventArgs e)
        {
            WindowState = WindowState.Minimized;
        }

        private void MaximizeButton_Click(object sender, RoutedEventArgs e)
        {
            if (WindowState == WindowState.Maximized)
            {
                WindowState = WindowState.Normal;
                MaximizeBtn.Content = "□";
            }
            else
            {
                WindowState = WindowState.Maximized;
                MaximizeBtn.Content = "❐";
            }
        }

        private void MainWindow_SourceInitialized(object? sender, EventArgs e)
        {
            var helper = new WindowInteropHelper(this);
            IntPtr hwnd = helper.Handle;
            _hotkeyHwndSource = HwndSource.FromHwnd(hwnd);
            _hotkeyHwndSource?.AddHook(HotkeyWndProc);
            RegisterMenuToggleHotkey();

            _hotkeyRefreshTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(1.5)
            };
            _hotkeyRefreshTimer.Tick += (_, _) => RegisterMenuToggleHotkey();
            _hotkeyRefreshTimer.Start();
        }

        private IntPtr HotkeyWndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            if (msg == WM_HOTKEY && wParam.ToInt32() == MENU_TOGGLE_HOTKEY_ID)
            {
                handled = true;
                Dispatcher.BeginInvoke(() =>
                {
                    if (IsVisible)
                    {
                        Hide();
                    }
                    else
                    {
                        Show();
                        Activate();
                    }
                });
            }
            return IntPtr.Zero;
        }

        private void RegisterMenuToggleHotkey()
        {
            if (_hotkeyHwndSource == null) return;
            IntPtr hwnd = _hotkeyHwndSource.Handle;
            _sharedMemory?.ReadConfig();
            int menuKey = _sharedMemory?.Config.MenuKey ?? 0;

            if (menuKey == _registeredMenuKey) return;

            if (_registeredMenuKey >= 0)
            {
                UnregisterHotKey(hwnd, MENU_TOGGLE_HOTKEY_ID);
                _registeredMenuKey = -1;
            }

            if (menuKey != 0)
            {
                if (RegisterHotKey(hwnd, MENU_TOGGLE_HOTKEY_ID, MOD_NOREPEAT, (uint)menuKey))
                    _registeredMenuKey = menuKey;
            }
        }

        protected override void OnClosed(EventArgs e)
        {
            _hotkeyRefreshTimer?.Stop();
            if (_hotkeyHwndSource != null && _registeredMenuKey >= 0)
            {
                try { UnregisterHotKey(_hotkeyHwndSource.Handle, MENU_TOGGLE_HOTKEY_ID); } catch { }
            }
            _statusTimer?.Stop();
            _sharedMemory?.Dispose();
            base.OnClosed(e);
        }
    }
}
