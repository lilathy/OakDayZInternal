using System;
using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;
using System.Security;

namespace OakPanel
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct SharedConfig
    {
        public uint Magic;              
        public uint Version;
        
        
        [MarshalAs(UnmanagedType.I1)]
        public bool EspEnabled;
        [MarshalAs(UnmanagedType.I1)]
        public bool EspPlayers;
        [MarshalAs(UnmanagedType.I1)]
        public bool EspZombies;
        [MarshalAs(UnmanagedType.I1)]
        public bool EspAnimals;
        [MarshalAs(UnmanagedType.I1)]
        public bool EspItems;
        [MarshalAs(UnmanagedType.I1)]
        public bool EspVehicles;
        [MarshalAs(UnmanagedType.I1)]
        public bool EspSkeleton;
        [MarshalAs(UnmanagedType.I1)]
        public bool EspBox;
        [MarshalAs(UnmanagedType.I1)]
        public bool EspName;
        [MarshalAs(UnmanagedType.I1)]
        public bool EspDistance;
        [MarshalAs(UnmanagedType.I1)]
        public bool EspHealth;
        
        
        [MarshalAs(UnmanagedType.I1)]
        public bool PlayerBox;
        [MarshalAs(UnmanagedType.I1)]
        public bool PlayerName;
        [MarshalAs(UnmanagedType.I1)]
        public bool PlayerDistance;
        [MarshalAs(UnmanagedType.I1)]
        public bool ZombieBox;
        [MarshalAs(UnmanagedType.I1)]
        public bool ZombieName;
        [MarshalAs(UnmanagedType.I1)]
        public bool ZombieDistance;
        [MarshalAs(UnmanagedType.I1)]
        public bool AnimalBox;
        [MarshalAs(UnmanagedType.I1)]
        public bool AnimalName;
        [MarshalAs(UnmanagedType.I1)]
        public bool AnimalDistance;
        [MarshalAs(UnmanagedType.I1)]
        public bool ItemBox;
        [MarshalAs(UnmanagedType.I1)]
        public bool ItemName;
        [MarshalAs(UnmanagedType.I1)]
        public bool ItemDistance;
        [MarshalAs(UnmanagedType.I1)]
        public bool VehicleBox;
        [MarshalAs(UnmanagedType.I1)]
        public bool VehicleName;
        [MarshalAs(UnmanagedType.I1)]
        public bool VehicleDistance;
        
        
        public int PlayerMaxDistance;
        public int ZombieMaxDistance;
        public int AnimalMaxDistance;
        public int ItemMaxDistance;
        public int VehicleMaxDistance;
        
        
        public int MaxPlayers;
        public int MaxZombies;
        public int MaxAnimals;
        public int MaxItems;
        
        
        public uint ColorPlayerBox;
        public uint ColorPlayerSkeleton;
        public uint ColorZombieBox;
        public uint ColorZombieSkeleton;
        public uint ColorAnimalBox;
        public uint ColorItemBox;
        public uint ColorVehicleBox;
        public uint ColorPlayerName;
        public uint ColorZombieName;
        public uint ColorAnimalName;
        public uint ColorItemName;
        public uint ColorVehicleName;
        
        
        [MarshalAs(UnmanagedType.I1)]
        public bool AimbotEnabled;
        [MarshalAs(UnmanagedType.I1)]
        public bool AimbotPlayers;
        [MarshalAs(UnmanagedType.I1)]
        public bool AimbotZombies;
        [MarshalAs(UnmanagedType.I1)]
        public bool AimbotAnimals;
        public int AimbotFov;
        public int AimbotSmooth;
        public int AimbotBone;          
        [MarshalAs(UnmanagedType.I1)]
        public bool AimbotVisCheck;
        [MarshalAs(UnmanagedType.I1)]
        public bool AimbotPrediction;
        public int AimbotKey;           
        
        
        [MarshalAs(UnmanagedType.I1)]
        public bool FastBullets;
        
        [MarshalAs(UnmanagedType.I1)]
        public bool NoRecoil;
        [MarshalAs(UnmanagedType.I1)]
        public bool NoSway;
        [MarshalAs(UnmanagedType.I1)]
        public bool InfiniteStamina;
        [MarshalAs(UnmanagedType.I1)]
        public bool SpeedHack;
        public float SpeedMultiplier;
        [MarshalAs(UnmanagedType.I1)]
        public bool NoGrass;
        [MarshalAs(UnmanagedType.I1)]
        public bool NightVision;
        [MarshalAs(UnmanagedType.I1)]
        public bool Crosshair;
        
        
        public int MenuKey;
        [MarshalAs(UnmanagedType.I1)]
        public bool StreamProof;
        
        
        public float PlayerX;
        public float PlayerY;
        public float PlayerZ;
        
        
        [MarshalAs(UnmanagedType.I1)]
        public bool DllAttached;
        public uint TargetPid;
    }

    public class SharedMemoryManager : IDisposable
    {
        private const string SharedMemName = "OakPanelSharedMem";
        private const int SharedMemSize = 4096;
        private const uint MagicValue = 0x4F414B00;

        private MemoryMappedFile? _mmf;
        private MemoryMappedViewAccessor? _accessor;
        private bool _disposed = false;

        public SharedConfig Config { get; private set; }

        public bool Initialize()
        {
            try
            {
                for (int attempt = 0; attempt < 3; attempt++)
                {
                    try
                    {
                        _mmf = MemoryMappedFile.CreateOrOpen(SharedMemName, SharedMemSize, MemoryMappedFileAccess.ReadWrite);
                        _accessor = _mmf.CreateViewAccessor(0, SharedMemSize, MemoryMappedFileAccess.ReadWrite);

                        uint magic = _accessor.ReadUInt32(0);
                        
                        if (magic != MagicValue)
                        {
                            InitializeDefaults();
                            WriteConfig();
                            ReadConfig();
                        }
                        else
                        {
                            ReadConfig();
                            FixZeroColors();
                        }

                        return true;
                    }
                    catch (System.IO.IOException)
                    {
                        if (attempt < 2)
                        {
                            System.Threading.Thread.Sleep(50);
                            continue;
                        }
                        throw;
                    }
                }
                return false;
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"shared mem init failed: {ex.Message}");
                return false;
            }
        }

        private void FixZeroColors()
        {
            var config = Config;
            bool needsFix = false;
            
            if (config.ColorPlayerBox == 0) { config.ColorPlayerBox = 0x0000FF00; needsFix = true; }
            if (config.ColorPlayerSkeleton == 0) { config.ColorPlayerSkeleton = 0x00FFFFFF; needsFix = true; }
            if (config.ColorZombieBox == 0) { config.ColorZombieBox = 0x00FF0000; needsFix = true; }
            if (config.ColorZombieSkeleton == 0) { config.ColorZombieSkeleton = 0x00FF8000; needsFix = true; }
            if (config.ColorAnimalBox == 0) { config.ColorAnimalBox = 0x00FFA500; needsFix = true; }
            if (config.ColorItemBox == 0) { config.ColorItemBox = 0x0000FFFF; needsFix = true; }
            if (config.ColorVehicleBox == 0) { config.ColorVehicleBox = 0x00FF00FF; needsFix = true; }
            if (config.ColorPlayerName == 0) { config.ColorPlayerName = 0x0000FF00; needsFix = true; }
            if (config.ColorZombieName == 0) { config.ColorZombieName = 0x00FF6400; needsFix = true; }
            if (config.ColorAnimalName == 0) { config.ColorAnimalName = 0x00FFA500; needsFix = true; }
            if (config.ColorItemName == 0) { config.ColorItemName = 0x0000FFFF; needsFix = true; }
            if (config.ColorVehicleName == 0) { config.ColorVehicleName = 0x00FF00FF; needsFix = true; }
            
            if (needsFix)
            {
                Config = config;
                WriteConfig();
            }
        }

        [SecuritySafeCritical]
        public void ReadConfig()
        {
            if (_accessor == null) return;

            try
            {
                byte[] buffer = new byte[Marshal.SizeOf<SharedConfig>()];
                _accessor.ReadArray(0, buffer, 0, buffer.Length);
                
                unsafe
                {
                    fixed (byte* ptr = buffer)
                    {
                        Config = Marshal.PtrToStructure<SharedConfig>((IntPtr)ptr);
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"read config failed: {ex.Message}");
            }
        }

        [SecuritySafeCritical]
        public void WriteConfig()
        {
            if (_accessor == null) return;

            try
            {
                var config = Config;
                if (config.Magic != MagicValue)
                {
                    config.Magic = MagicValue;
                    if (config.Version == 0)
                        config.Version = 1;
                }
                
                
                EnsureValidColors(ref config);
                Config = config;

                byte[] buffer = new byte[Marshal.SizeOf<SharedConfig>()];
                unsafe
                {
                    fixed (byte* ptr = buffer)
                    {
                        Marshal.StructureToPtr(Config, (IntPtr)ptr, false);
                    }
                }
                _accessor.WriteArray(0, buffer, 0, buffer.Length);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"write config failed: {ex.Message}");
            }
        }

        private void EnsureValidColors(ref SharedConfig config)
        {
            if (config.ColorPlayerBox == 0) config.ColorPlayerBox = 0x0000FF00;
            if (config.ColorPlayerSkeleton == 0) config.ColorPlayerSkeleton = 0x00FFFFFF;
            if (config.ColorZombieBox == 0) config.ColorZombieBox = 0x00FF0000;
            if (config.ColorZombieSkeleton == 0) config.ColorZombieSkeleton = 0x00FF8000;
            if (config.ColorAnimalBox == 0) config.ColorAnimalBox = 0x00FFA500;
            if (config.ColorItemBox == 0) config.ColorItemBox = 0x0000FFFF;
            if (config.ColorVehicleBox == 0) config.ColorVehicleBox = 0x00FF00FF;
            if (config.ColorPlayerName == 0) config.ColorPlayerName = 0x0000FF00;
            if (config.ColorZombieName == 0) config.ColorZombieName = 0x00FF6400;
            if (config.ColorAnimalName == 0) config.ColorAnimalName = 0x00FFA500;
            if (config.ColorItemName == 0) config.ColorItemName = 0x0000FFFF;
            if (config.ColorVehicleName == 0) config.ColorVehicleName = 0x00FF00FF;
        }

        public void UpdateConfig(Func<SharedConfig, SharedConfig> updater)
        {
            var config = updater(Config);
            if (config.Magic != MagicValue)
            {
                config.Magic = MagicValue;
                if (config.Version == 0)
                    config.Version = 1;
            }
            Config = config;
            WriteConfig();
        }

        public void ResetToDefaults()
        {
            InitializeDefaults();
            WriteConfig();
        }

        private void InitializeDefaults()
        {
            Config = new SharedConfig
            {
                Magic = MagicValue,
                Version = 1,
                
                
                EspEnabled = true,
                EspPlayers = true,
                EspZombies = true,
                EspAnimals = false,
                EspItems = false,
                EspVehicles = true,
                EspSkeleton = true,
                EspBox = true,
                EspName = true,
                EspDistance = true,
                EspHealth = false,
                
                
                PlayerBox = true,
                PlayerName = true,
                PlayerDistance = true,
                ZombieBox = true,
                ZombieName = false,
                ZombieDistance = true,
                AnimalBox = true,
                AnimalName = false,
                AnimalDistance = true,
                ItemBox = true,
                ItemName = true,
                ItemDistance = true,
                VehicleBox = true,
                VehicleName = false,
                VehicleDistance = true,
                
                
                PlayerMaxDistance = 1000,
                ZombieMaxDistance = 300,
                AnimalMaxDistance = 200,
                ItemMaxDistance = 100,
                VehicleMaxDistance = 500,
                
                
                MaxPlayers = 50,
                MaxZombies = 30,
                MaxAnimals = 20,
                MaxItems = 50,
                
                
                ColorPlayerBox = 0x0000FF00,      
                ColorPlayerSkeleton = 0x00FFFFFF, 
                ColorZombieBox = 0x00FF0000,      
                ColorZombieSkeleton = 0x00FF8000, 
                ColorAnimalBox = 0x00FFA500,      
                ColorItemBox = 0x0000FFFF,        
                ColorVehicleBox = 0x00FF00FF,     
                ColorPlayerName = 0x0000FF00,     
                ColorZombieName = 0x00FF6400,     
                ColorAnimalName = 0x00FFA500,     
                ColorItemName = 0x0000FFFF,       
                ColorVehicleName = 0x00FF00FF,    
                
                
                AimbotEnabled = false,
                AimbotPlayers = true,
                AimbotZombies = false,
                AimbotAnimals = false,
                AimbotFov = 15,
                AimbotSmooth = 5,
                AimbotBone = 0,  
                AimbotVisCheck = true,
                AimbotPrediction = true,
                AimbotKey = 0x02,
                FastBullets = true, 
                
                
                NoRecoil = false,
                NoSway = false,
                InfiniteStamina = false,
                SpeedHack = false,
                SpeedMultiplier = 1.5f,
                NoGrass = false,
                NightVision = false,
                Crosshair = false,
                
                
                MenuKey = 0x2D, 
                StreamProof = false,
                
                
                PlayerX = 0.0f,
                PlayerY = 0.0f,
                PlayerZ = 0.0f
            };
        }

        public void Dispose()
        {
            if (_disposed) return;

            _accessor?.Dispose();
            _mmf?.Dispose();
            _disposed = true;
        }
    }
}
