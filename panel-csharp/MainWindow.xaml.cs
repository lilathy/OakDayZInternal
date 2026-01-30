using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Threading;
using OakPanel.Views;

namespace OakPanel
{
    public partial class MainWindow : Window
    {
        private SharedMemoryManager? _sharedMemory;
        private int _currentTab = 0;
        private DispatcherTimer? _statusTimer;
        private Button[] _tabButtons = Array.Empty<Button>();

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

        protected override void OnClosed(EventArgs e)
        {
            _statusTimer?.Stop();
            _sharedMemory?.Dispose();
            base.OnClosed(e);
        }
    }
}
