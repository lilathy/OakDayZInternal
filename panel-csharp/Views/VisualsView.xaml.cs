using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace OakPanel.Views
{
    public partial class VisualsView : UserControl, INotifyPropertyChanged
    {
        private readonly SharedMemoryManager _sharedMemory;
        private bool _espEnabled;
        private ObservableCollection<EspToggleItem> _espToggles = new();
        private ObservableCollection<object> _playerSettings = new();
        private ObservableCollection<object> _zombieSettings = new();
        private ObservableCollection<object> _animalSettings = new();
        private ObservableCollection<object> _itemSettings = new();
        private ObservableCollection<object> _vehicleSettings = new();

        public bool EspEnabled
        {
            get => _espEnabled;
            set
            {
                if (_espEnabled != value)
                {
                    _espEnabled = value;
                    OnPropertyChanged();
                    _sharedMemory.UpdateConfig(cfg => { cfg.EspEnabled = value; return cfg; });
                }
            }
        }

        public ObservableCollection<EspToggleItem> EspToggles
        {
            get => _espToggles;
            set { _espToggles = value; OnPropertyChanged(); }
        }

        public ObservableCollection<object> PlayerSettings
        {
            get => _playerSettings;
            set { _playerSettings = value; OnPropertyChanged(); }
        }

        public ObservableCollection<object> ZombieSettings
        {
            get => _zombieSettings;
            set { _zombieSettings = value; OnPropertyChanged(); }
        }

        public ObservableCollection<object> AnimalSettings
        {
            get => _animalSettings;
            set { _animalSettings = value; OnPropertyChanged(); }
        }

        public ObservableCollection<object> ItemSettings
        {
            get => _itemSettings;
            set { _itemSettings = value; OnPropertyChanged(); }
        }

        public ObservableCollection<object> VehicleSettings
        {
            get => _vehicleSettings;
            set { _vehicleSettings = value; OnPropertyChanged(); }
        }

        public VisualsView(SharedMemoryManager sharedMemory)
        {
            InitializeComponent();
            _sharedMemory = sharedMemory;
            DataContext = this;
            LoadSettings();
        }

        private void LoadSettings()
        {
            var config = _sharedMemory.Config;
            _espEnabled = config.EspEnabled;

            
            EspToggles.Clear();
            EspToggles.Add(new EspToggleItem("Players", "Show player ESP", config.EspPlayers,
                () => { _sharedMemory.UpdateConfig(cfg => { cfg.EspPlayers = !cfg.EspPlayers; return cfg; }); }));
            EspToggles.Add(new EspToggleItem("Zombies", "Show zombie/infected ESP", config.EspZombies,
                () => { _sharedMemory.UpdateConfig(cfg => { cfg.EspZombies = !cfg.EspZombies; return cfg; }); }));
            EspToggles.Add(new EspToggleItem("Animals", "Show animal ESP", config.EspAnimals,
                () => { _sharedMemory.UpdateConfig(cfg => { cfg.EspAnimals = !cfg.EspAnimals; return cfg; }); }));
            EspToggles.Add(new EspToggleItem("Items", "Show loot ESP", config.EspItems,
                () => { _sharedMemory.UpdateConfig(cfg => { cfg.EspItems = !cfg.EspItems; return cfg; }); }));
            EspToggles.Add(new EspToggleItem("Vehicles", "Show vehicle ESP", config.EspVehicles,
                () => { _sharedMemory.UpdateConfig(cfg => { cfg.EspVehicles = !cfg.EspVehicles; return cfg; }); }));

            
            PlayerSettings.Clear();
            PlayerSettings.Add(new ToggleSetting("Show Boxes", "Display bounding boxes", 
                config.PlayerBox,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.PlayerBox = v; return cfg; }); }));
            PlayerSettings.Add(new ToggleSetting("Show Names", "Display player names", 
                config.PlayerName,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.PlayerName = v; return cfg; }); }));
            PlayerSettings.Add(new ToggleSetting("Show Distance", "Display distance in meters", 
                config.PlayerDistance,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.PlayerDistance = v; return cfg; }); }));
            PlayerSettings.Add(new ToggleSetting("Show Skeleton", "Display bone skeleton", 
                config.EspSkeleton,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.EspSkeleton = v; return cfg; }); }));
            PlayerSettings.Add(new DistanceSetting("Max Distance", "Maximum render distance", 
                config.PlayerMaxDistance, 50, 1500, 
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.PlayerMaxDistance = v; return cfg; }); }));
            PlayerSettings.Add(new ColorSetting("Box Color", "Color for boxes", 
                config.ColorPlayerBox,
                (c) => { _sharedMemory.UpdateConfig(cfg => { cfg.ColorPlayerBox = c; return cfg; }); }));
            PlayerSettings.Add(new ColorSetting("Skeleton Color", "Color for skeleton", 
                config.ColorPlayerSkeleton,
                (c) => { _sharedMemory.UpdateConfig(cfg => { cfg.ColorPlayerSkeleton = c; return cfg; }); }));
            PlayerSettings.Add(new ColorSetting("Name Color", "Color for names", 
                config.ColorPlayerName,
                (c) => { _sharedMemory.UpdateConfig(cfg => { cfg.ColorPlayerName = c; return cfg; }); }));

            
            ZombieSettings.Clear();
            ZombieSettings.Add(new ToggleSetting("Show Boxes", "Display bounding boxes", 
                config.ZombieBox,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.ZombieBox = v; return cfg; }); }));
            ZombieSettings.Add(new ToggleSetting("Show Names", "Display zombie type", 
                config.ZombieName,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.ZombieName = v; return cfg; }); }));
            ZombieSettings.Add(new ToggleSetting("Show Distance", "Display distance in meters", 
                config.ZombieDistance,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.ZombieDistance = v; return cfg; }); }));
            ZombieSettings.Add(new DistanceSetting("Max Distance", "Maximum render distance",
                config.ZombieMaxDistance, 50, 500,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.ZombieMaxDistance = v; return cfg; }); }));
            ZombieSettings.Add(new ColorSetting("Box Color", "Color for boxes", 
                config.ColorZombieBox,
                (c) => { _sharedMemory.UpdateConfig(cfg => { cfg.ColorZombieBox = c; return cfg; }); }));
            ZombieSettings.Add(new ColorSetting("Name Color", "Color for names", 
                config.ColorZombieName,
                (c) => { _sharedMemory.UpdateConfig(cfg => { cfg.ColorZombieName = c; return cfg; }); }));

            
            AnimalSettings.Clear();
            AnimalSettings.Add(new ToggleSetting("Show Boxes", "Display bounding boxes", 
                config.AnimalBox,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.AnimalBox = v; return cfg; }); }));
            AnimalSettings.Add(new ToggleSetting("Show Names", "Display animal type", 
                config.AnimalName,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.AnimalName = v; return cfg; }); }));
            AnimalSettings.Add(new ToggleSetting("Show Distance", "Display distance in meters", 
                config.AnimalDistance,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.AnimalDistance = v; return cfg; }); }));
            AnimalSettings.Add(new DistanceSetting("Max Distance", "Maximum render distance",
                config.AnimalMaxDistance, 50, 500,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.AnimalMaxDistance = v; return cfg; }); }));
            AnimalSettings.Add(new ColorSetting("Box Color", "Color for boxes", 
                config.ColorAnimalBox,
                (c) => { _sharedMemory.UpdateConfig(cfg => { cfg.ColorAnimalBox = c; return cfg; }); }));
            AnimalSettings.Add(new ColorSetting("Name Color", "Color for names", 
                config.ColorAnimalName,
                (c) => { _sharedMemory.UpdateConfig(cfg => { cfg.ColorAnimalName = c; return cfg; }); }));

            
            ItemSettings.Clear();
            ItemSettings.Add(new ToggleSetting("Show Boxes", "Display bounding boxes", 
                config.ItemBox,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.ItemBox = v; return cfg; }); }));
            ItemSettings.Add(new ToggleSetting("Show Names", "Display item names", 
                config.ItemName,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.ItemName = v; return cfg; }); }));
            ItemSettings.Add(new ToggleSetting("Show Distance", "Display distance in meters", 
                config.ItemDistance,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.ItemDistance = v; return cfg; }); }));
            ItemSettings.Add(new DistanceSetting("Max Distance", "Maximum render distance",
                config.ItemMaxDistance, 10, 200,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.ItemMaxDistance = v; return cfg; }); }));
            ItemSettings.Add(new ColorSetting("Box Color", "Color for boxes", 
                config.ColorItemBox,
                (c) => { _sharedMemory.UpdateConfig(cfg => { cfg.ColorItemBox = c; return cfg; }); }));
            ItemSettings.Add(new ColorSetting("Name Color", "Color for names", 
                config.ColorItemName,
                (c) => { _sharedMemory.UpdateConfig(cfg => { cfg.ColorItemName = c; return cfg; }); }));

            
            VehicleSettings.Clear();
            VehicleSettings.Add(new ToggleSetting("Show Boxes", "Display bounding boxes", 
                config.VehicleBox,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.VehicleBox = v; return cfg; }); }));
            VehicleSettings.Add(new ToggleSetting("Show Names", "Display vehicle names", 
                config.VehicleName,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.VehicleName = v; return cfg; }); }));
            VehicleSettings.Add(new ToggleSetting("Show Distance", "Display distance in meters", 
                config.VehicleDistance,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.VehicleDistance = v; return cfg; }); }));
            VehicleSettings.Add(new DistanceSetting("Max Distance", "Maximum render distance",
                config.VehicleMaxDistance, 50, 1000,
                (v) => { _sharedMemory.UpdateConfig(cfg => { cfg.VehicleMaxDistance = v; return cfg; }); }));
            VehicleSettings.Add(new ColorSetting("Box Color", "Color for boxes", 
                config.ColorVehicleBox,
                (c) => { _sharedMemory.UpdateConfig(cfg => { cfg.ColorVehicleBox = c; return cfg; }); }));
            VehicleSettings.Add(new ColorSetting("Name Color", "Color for names", 
                config.ColorVehicleName,
                (c) => { _sharedMemory.UpdateConfig(cfg => { cfg.ColorVehicleName = c; return cfg; }); }));
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    public class EspToggleItem : INotifyPropertyChanged
    {
        private bool _isChecked;

        public string Label { get; set; }
        public string Description { get; set; }
        public bool IsChecked
        {
            get => _isChecked;
            set
            {
                if (_isChecked != value)
                {
                    _isChecked = value;
                    OnPropertyChanged();
                    OnToggled?.Invoke();
                }
            }
        }
        public Action? OnToggled { get; set; }

        public EspToggleItem(string label, string desc, bool initial, Action? onToggled = null)
        {
            Label = label;
            Description = desc;
            _isChecked = initial;
            OnToggled = onToggled;
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    public class DistanceSetting : INotifyPropertyChanged
    {
        private int _value;

        public string Label { get; set; }
        public string Description { get; set; }
        public int Min { get; set; }
        public int Max { get; set; }
        public int Value
        {
            get => _value;
            set
            {
                if (_value != value)
                {
                    _value = value;
                    OnPropertyChanged();
                    OnValueChanged?.Invoke(value);
                }
            }
        }
        public Action<int>? OnValueChanged { get; set; }

        public DistanceSetting(string label, string desc, int initial, int min, int max, Action<int>? onChanged = null)
        {
            Label = label;
            Description = desc;
            _value = initial;
            Min = min;
            Max = max;
            OnValueChanged = onChanged;
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    public class ToggleSetting : INotifyPropertyChanged
    {
        private bool _isChecked;

        public string Label { get; set; }
        public string Description { get; set; }
        public bool IsChecked
        {
            get => _isChecked;
            set
            {
                if (_isChecked != value)
                {
                    _isChecked = value;
                    OnPropertyChanged();
                    OnToggled?.Invoke(value);
                }
            }
        }
        public Action<bool>? OnToggled { get; set; }

        public ToggleSetting(string label, string desc, bool initial, Action<bool>? onToggled = null)
        {
            Label = label;
            Description = desc;
            _isChecked = initial;
            OnToggled = onToggled;
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    public class ColorSetting : INotifyPropertyChanged
    {
        private SolidColorBrush _colorBrush;
        private string _hexString;

        public string Label { get; set; }
        public string Description { get; set; }
        public SolidColorBrush ColorBrush
        {
            get => _colorBrush;
            private set
            {
                _colorBrush = value;
                OnPropertyChanged();
            }
        }
        public string HexString
        {
            get => _hexString;
            set
            {
                if (_hexString != value)
                {
                    _hexString = value;
                    OnPropertyChanged();
                    TryUpdateColorFromHex(value);
                }
            }
        }
        public Action<uint>? OnColorChanged { get; set; }

        public ColorSetting(string label, string description, uint colorValue, Action<uint>? onChanged = null)
        {
            Label = label;
            Description = description;
            var r = (byte)((colorValue >> 16) & 0xFF);
            var g = (byte)((colorValue >> 8) & 0xFF);
            var b = (byte)(colorValue & 0xFF);
            _colorBrush = new SolidColorBrush(Color.FromRgb(r, g, b));
            _hexString = $"#{r:X2}{g:X2}{b:X2}";
            OnColorChanged = onChanged;
        }

        private void TryUpdateColorFromHex(string hex)
        {
            try
            {
                hex = hex.TrimStart('#');
                
                if (hex.Length == 3)
                {
                    hex = $"{hex[0]}{hex[0]}{hex[1]}{hex[1]}{hex[2]}{hex[2]}";
                }
                
                if (hex.Length == 6)
                {
                    var r = Convert.ToByte(hex.Substring(0, 2), 16);
                    var g = Convert.ToByte(hex.Substring(2, 2), 16);
                    var b = Convert.ToByte(hex.Substring(4, 2), 16);
                    
                    ColorBrush = new SolidColorBrush(Color.FromRgb(r, g, b));
                    uint newValue = (uint)((r << 16) | (g << 8) | b);
                    OnColorChanged?.Invoke(newValue);
                }
            }
            catch
            {
                
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }
}
