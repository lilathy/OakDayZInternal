
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")


extern "C" {
    int _fltused = 0;
    
    
    EXCEPTION_DISPOSITION __C_specific_handler(
        struct _EXCEPTION_RECORD* ExceptionRecord,
        void* EstablisherFrame,
        struct _CONTEXT* ContextRecord,
        struct _DISPATCHER_CONTEXT* DispatcherContext)
    {
        return ExceptionContinueSearch;
    }
}


static int StrCmp(const char* s1, const char* s2)
{
    while (*s1 && *s2)
    {
        if (*s1 != *s2) return *s1 - *s2;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}


static int StrCmpI(const char* s1, const char* s2)
{
    while (*s1 && *s2)
    {
        char c1 = *s1;
        char c2 = *s2;
        
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}


static bool StrContainsI(const char* haystack, const char* needle)
{
    const char* h = haystack;
    while (*h)
    {
        const char* n = needle;
        const char* h2 = h;
        bool match = true;
        while (*n && *h2)
        {
            char c1 = *h2;
            char c2 = *n;
            if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
            if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
            if (c1 != c2)
            {
                match = false;
                break;
            }
            h2++;
            n++;
        }
        if (match && !*n) return true;  
        h++;
    }
    return false;
}


static bool IsAnimal(const char* configName)
{
    if (!configName || !configName[0]) return false;
    
    
    const char* animalNames[] = {
        "cow", "deer", "boar", "chicken", "goat", "sheep", 
        "wolf", "bear", "rabbit", "pig", "horse", "rooster",
        "hen", "bull", "cattle", "stag", "doe", "wildboar",
        "mouflon", "ibex", "lynx", "fox", "carp", "mackerel",
        "salmon", "pike", "bass", "catfish", "fish",
        "animal", "creature", "beast", "livestock"
    };
    
    for (int i = 0; i < sizeof(animalNames) / sizeof(animalNames[0]); i++)
    {
        if (StrContainsI(configName, animalNames[i]))
            return true;
    }
    
    return false;
}


static bool IsExcludedFromItems(const char* configName)
{
    if (!configName || !configName[0]) return false;
    
    
    const char* excludedNames[] = {
        "gardenbed", "garden", "garden_bed", "gardenbed", "bed",
        "barrel", "barrelhre", "tent", "car", "truck", "boat", 
        "helicopter", "plane", "building", "house", "shed", "garage", 
        "fence", "wall", "gate", "door", "window", "ladder", "stairs", 
        "roof", "container", "crate", "box", "locker", "cabinet", "shelf",
        "fireplace", "stove", "oven", "fridge", "freezer",
        "well", "pump", "generator", "solar", "battery",
        "flag", "pole", "post", "sign", "lamp", "light",
        "tree", "bush", "rock", "stone", "grass", "plant",
        "structure", "base", "foundation", "platform", "tower"
    };
    
    for (int i = 0; i < sizeof(excludedNames) / sizeof(excludedNames[0]); i++)
    {
        if (StrContainsI(configName, excludedNames[i]))
            return true;
    }
    
    return false;
}


static const unsigned char g_VS[] = {
    0x44, 0x58, 0x42, 0x43, 0xBE, 0x7F, 0xFA, 0x49, 0xF7, 0xCE, 0xE6, 0x00, 0x78, 0x87, 0x89, 0x7A, 
    0xA8, 0x34, 0x63, 0x66, 0x01, 0x00, 0x00, 0x00, 0x74, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 
    0x34, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x44, 0x01, 0x00, 0x00, 
    0xD8, 0x01, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x05, 0xFE, 0xFF, 
    0x00, 0x01, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x52, 0x44, 0x31, 0x31, 0x3C, 0x00, 0x00, 0x00, 
    0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 
    0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 
    0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 
    0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00, 
    0x49, 0x53, 0x47, 0x4E, 0x48, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 
    0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 
    0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0xAB, 
    0x4F, 0x53, 0x47, 0x4E, 0x4C, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 
    0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 
    0x53, 0x56, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 
    0x52, 0x00, 0xAB, 0xAB, 0x53, 0x48, 0x45, 0x58, 0x8C, 0x00, 0x00, 0x00, 0x50, 0x00, 0x01, 0x00, 
    0x23, 0x00, 0x00, 0x00, 0x6A, 0x08, 0x00, 0x01, 0x5F, 0x00, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0xF2, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 
    0x67, 0x00, 0x00, 0x04, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
    0x65, 0x00, 0x00, 0x03, 0xF2, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 
    0x32, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x36, 0x00, 0x00, 0x08, 0xC2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 
    0x36, 0x00, 0x00, 0x05, 0xF2, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x1E, 0x10, 0x00, 
    0x01, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41, 0x54, 0x94, 0x00, 0x00, 0x00, 
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00
};


static const unsigned char g_PS[] = {
    0x44, 0x58, 0x42, 0x43, 0x29, 0x32, 0x26, 0xA8, 0x0C, 0xD2, 0xDF, 0x85, 0x93, 0x8E, 0x4A, 0x0F, 
    0x51, 0x32, 0xC0, 0xAE, 0x01, 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 
    0x34, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0xF4, 0x00, 0x00, 0x00, 0x28, 0x01, 0x00, 0x00, 
    0x6C, 0x01, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x05, 0xFF, 0xFF, 
    0x00, 0x01, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x52, 0x44, 0x31, 0x31, 0x3C, 0x00, 0x00, 0x00, 
    0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 
    0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 
    0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 
    0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00, 
    0x49, 0x53, 0x47, 0x4E, 0x4C, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 
    0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 
    0x53, 0x56, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 
    0x52, 0x00, 0xAB, 0xAB, 0x4F, 0x53, 0x47, 0x4E, 0x2C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
    0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x54, 
    0x61, 0x72, 0x67, 0x65, 0x74, 0x00, 0xAB, 0xAB, 0x53, 0x48, 0x45, 0x58, 0x3C, 0x00, 0x00, 0x00, 
    0x50, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x6A, 0x08, 0x00, 0x01, 0x62, 0x10, 0x00, 0x03, 
    0xF2, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xF2, 0x20, 0x10, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x46, 0x1E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41, 0x54, 
    0x94, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


namespace offsets {
    namespace modbase {
        constexpr uintptr_t World = 0xF4B050;         
        constexpr uintptr_t NetworkManager = 0xF5E190;
        constexpr uintptr_t DayZPlayer = 0x428E6F8;   
    }
    namespace world {
        
        constexpr uintptr_t Camera = 0x1B8;
        constexpr uintptr_t LocalPlayer = 0x2960;
        constexpr uintptr_t NearEntList = 0xF48;      
        constexpr uintptr_t FarEntList = 0x1090;      
        constexpr uintptr_t SlowEntList = 0x2010;     
        constexpr uintptr_t BulletList = 0xE00;
        constexpr uintptr_t NoGrass = 0xC00;
    }
    namespace entity {
        constexpr uintptr_t EntType = 0x180;          
        constexpr uintptr_t VisualState = 0x1C8;      
        constexpr uintptr_t Inventory = 0x658;        
        constexpr uintptr_t NetworkID = 0x6E4;        
        constexpr uintptr_t IsDead = 0xE2;            
        constexpr uintptr_t EntSize = 0x8;
        constexpr uintptr_t Skeleton = 0x7E8;         
        constexpr uintptr_t ZombieSkeleton = 0x678;   
    }
    namespace entitytype {
        constexpr uintptr_t TypeName = 0x70;          
        constexpr uintptr_t ConfigName = 0xA8;        
    }
    namespace human {
        constexpr uintptr_t VisualState = 0x1C8;      
    }
    namespace visual {
        constexpr uintptr_t Position = 0x2C;          
        constexpr uintptr_t DirX = 0x20;              
        constexpr uintptr_t DirY = 0x28;              
        constexpr uintptr_t Transform = 0x8;
    }
    
    namespace camera {
        constexpr uintptr_t InvertedViewRight = 0x8;
        constexpr uintptr_t InvertedViewUp = 0x14;
        constexpr uintptr_t InvertedViewForward = 0x20;
        constexpr uintptr_t InvertedViewTranslation = 0x2C;
        constexpr uintptr_t ViewPortSize = 0x58;
        constexpr uintptr_t GetProjectionD1 = 0xD0;
        constexpr uintptr_t GetProjectionD2 = 0xDC;
    }
    namespace skeleton {
        constexpr uintptr_t AnimClass1 = 0x98;        
        constexpr uintptr_t AnimClass2 = 0x28;        
    }
    namespace player {
        constexpr uintptr_t Skeleton = 0x7E8;
    }
    namespace infected {
        constexpr uintptr_t Skeleton = 0x678;
    }
    namespace animclass {
        constexpr uintptr_t MatrixArray = 0xBF0;
        constexpr uintptr_t MatrixB = 0x54;
    }
}


#define SHARED_MEM_NAME L"OakPanelSharedMem"

#pragma pack(push, 1)
struct SharedConfig {
    
    DWORD magic;              
    DWORD version;
    
    
    bool espEnabled;
    bool espPlayers;
    bool espZombies;
    bool espAnimals;
    bool espItems;
    bool espVehicles;
    bool espSkeleton;
    bool espBox;
    bool espName;
    bool espDistance;
    bool espHealth;
    
    
    bool playerBox;
    bool playerName;
    bool playerDistanceEnabled;
    bool zombieBox;
    bool zombieName;
    bool zombieDistanceEnabled;
    bool animalBox;
    bool animalName;
    bool animalDistanceEnabled;
    bool itemBox;
    bool itemName;
    bool itemDistanceEnabled;
    bool vehicleBox;
    bool vehicleName;
    bool vehicleDistanceEnabled;
    
    
    int playerDistance;
    int zombieDistance;
    int animalDistance;
    int itemDistance;
    int vehicleDistance;
    
    
    int maxPlayers;
    int maxZombies;
    int maxAnimals;
    int maxItems;
    
    
    DWORD colorPlayerBox;
    DWORD colorPlayerSkeleton;
    DWORD colorZombieBox;
    DWORD colorZombieSkeleton;
    DWORD colorAnimalBox;
    DWORD colorItemBox;
    DWORD colorVehicleBox;
    DWORD colorPlayerName;
    DWORD colorZombieName;
    DWORD colorAnimalName;
    DWORD colorItemName;
    DWORD colorVehicleName;
    
    
    bool aimbotEnabled;
    bool aimbotPlayers;
    bool aimbotZombies;
    int aimbotFov;
    int aimbotSmooth;
    int aimbotBone;
    bool aimbotVisCheck;
    
    
    bool fastBullets;
    bool noRecoil;
    bool noSway;
    bool infiniteStamina;
    bool speedHack;
    float speedMultiplier;
    
    
    int menuKey;
    bool streamProof;
    
    
    bool dllAttached;
    DWORD targetPid;
    
    
    float PlayerX;
    float PlayerY;
    float PlayerZ;
};
#pragma pack(pop)

static HANDLE g_hMapFile = NULL;
static SharedConfig* g_SharedConfig = NULL;


static HMODULE g_Module = NULL;
static HMODULE g_GameModule = NULL;
static bool g_Running = true;
static bool g_Initialized = false;
static bool g_RenderInit = false;
static bool g_ShowESP = true;
static bool g_ShowSkeleton = true;
static bool g_ShowNames = true;
static bool g_ShowDistance = true;
static bool g_ESPPlayers = true;
static bool g_ESPZombies = true;
static bool g_ESPAnimals = false;
static bool g_ESPItems = false;
static bool g_ESPVehicles = true;
static bool g_FastBullets = false;


static bool g_PlayerBox = true;
static bool g_PlayerName = true;
static bool g_PlayerDistanceEnabled = true;
static bool g_ZombieBox = true;
static bool g_ZombieName = true;
static bool g_ZombieDistanceEnabled = true;
static bool g_AnimalBox = true;
static bool g_AnimalName = true;
static bool g_AnimalDistanceEnabled = true;
static bool g_ItemBox = true;
static bool g_ItemName = true;
static bool g_ItemDistanceEnabled = true;
static bool g_VehicleBox = true;
static bool g_VehicleName = true;
static bool g_VehicleDistanceEnabled = true;

static int g_PlayerDistance = 1000;
static int g_ZombieDistance = 300;
static int g_AnimalDistance = 200;
static int g_ItemDistance = 100;
static int g_VehicleDistance = 500;
static int g_MaxPlayers = 50;
static int g_MaxZombies = 30;
static int g_MaxAnimals = 20;
static int g_MaxItems = 50;
static int g_FrameCount = 0;


static float g_ColorPlayerBox[4] = { 0.0f, 1.0f, 0.0f, 1.0f };      
static float g_ColorPlayerSkeleton[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; 
static float g_ColorZombieBox[4] = { 1.0f, 0.0f, 0.0f, 1.0f };      
static float g_ColorZombieSkeleton[4] = { 1.0f, 0.5f, 0.0f, 1.0f }; 
static float g_ColorAnimalBox[4] = { 1.0f, 0.65f, 0.0f, 1.0f };     
static float g_ColorItemBox[4] = { 0.0f, 1.0f, 1.0f, 1.0f };        
static float g_ColorVehicleBox[4] = { 1.0f, 0.0f, 1.0f, 1.0f };     


static IDXGISwapChain* g_SwapChain = NULL;
static ID3D11Device* g_Device = NULL;
static ID3D11DeviceContext* g_Context = NULL;
static ID3D11RenderTargetView* g_RenderTarget = NULL;


static ID3D11VertexShader* g_VertexShader = NULL;
static ID3D11PixelShader* g_PixelShader = NULL;
static ID3D11InputLayout* g_InputLayout = NULL;
static ID3D11Buffer* g_VertexBuffer = NULL;
static ID3D11BlendState* g_BlendState = NULL;
static ID3D11RasterizerState* g_RasterizerState = NULL;
static ID3D11DepthStencilState* g_DepthStencilState = NULL;


static float g_ScreenWidth = 1920.0f;
static float g_ScreenHeight = 1080.0f;


static float g_NDCScaleX = 2.0f / 1920.0f;   
static float g_NDCScaleY = 2.0f / 1080.0f;   
static float g_ScreenHalfW = 960.0f;          
static float g_ScreenHalfH = 540.0f;          


typedef HRESULT(WINAPI* tPresent)(IDXGISwapChain*, UINT, UINT);
static tPresent oPresent = NULL;
static void** g_SwapChainVTable = NULL;


static void Log(const char* msg)
{
    (void)msg;
}


static bool ConnectToPanel()
{
    if (g_SharedConfig) return true;  
    
    g_hMapFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEM_NAME);
    if (!g_hMapFile) return false;
    
    g_SharedConfig = (SharedConfig*)MapViewOfFile(g_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedConfig));
    if (!g_SharedConfig) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
        return false;
    }
    
    
    g_SharedConfig->dllAttached = true;
    g_SharedConfig->targetPid = GetCurrentProcessId();
    
    Log("connected to panel");
    return true;
}


static void ColorToFloat(DWORD color, float* out)
{
    out[0] = ((color >> 16) & 0xFF) / 255.0f;  
    out[1] = ((color >> 8) & 0xFF) / 255.0f;   
    out[2] = (color & 0xFF) / 255.0f;          
    out[3] = 1.0f;                              
}

static void UpdateFromPanel()
{
    
    if (!g_SharedConfig) {
        ConnectToPanel();
    }
    
    
    if (g_SharedConfig && g_SharedConfig->magic == 0x4F414B00) {
        
        if (g_SharedConfig->version == 0) return;
        
        
        g_ShowESP = g_SharedConfig->espEnabled;
        g_ESPPlayers = g_SharedConfig->espPlayers;
        g_ESPZombies = g_SharedConfig->espZombies;
        g_ESPAnimals = g_SharedConfig->espAnimals;
        g_ESPItems = g_SharedConfig->espItems;
        g_ESPVehicles = g_SharedConfig->espVehicles;
        g_ShowSkeleton = g_SharedConfig->espSkeleton;
        g_ShowNames = g_SharedConfig->espName;
        g_ShowDistance = g_SharedConfig->espDistance;
        
        
        g_PlayerBox = g_SharedConfig->playerBox;
        g_PlayerName = g_SharedConfig->playerName;
        g_PlayerDistanceEnabled = g_SharedConfig->playerDistanceEnabled;
        g_ZombieBox = g_SharedConfig->zombieBox;
        g_ZombieName = g_SharedConfig->zombieName;
        g_ZombieDistanceEnabled = g_SharedConfig->zombieDistanceEnabled;
        g_AnimalBox = g_SharedConfig->animalBox;
        g_AnimalName = g_SharedConfig->animalName;
        g_AnimalDistanceEnabled = g_SharedConfig->animalDistanceEnabled;
        g_ItemBox = g_SharedConfig->itemBox;
        g_ItemName = g_SharedConfig->itemName;
        g_ItemDistanceEnabled = g_SharedConfig->itemDistanceEnabled;
        g_VehicleBox = g_SharedConfig->vehicleBox;
        g_VehicleName = g_SharedConfig->vehicleName;
        g_VehicleDistanceEnabled = g_SharedConfig->vehicleDistanceEnabled;
        
        
        g_FastBullets = g_SharedConfig->fastBullets;
        
        
        if (g_SharedConfig->playerDistance > 0)
            g_PlayerDistance = g_SharedConfig->playerDistance;
        if (g_SharedConfig->zombieDistance > 0)
            g_ZombieDistance = g_SharedConfig->zombieDistance;
        if (g_SharedConfig->animalDistance > 0)
            g_AnimalDistance = g_SharedConfig->animalDistance;
        if (g_SharedConfig->itemDistance > 0)
            g_ItemDistance = g_SharedConfig->itemDistance;
        if (g_SharedConfig->vehicleDistance > 0)
            g_VehicleDistance = g_SharedConfig->vehicleDistance;
        
        
        if (g_SharedConfig->maxPlayers > 0)
            g_MaxPlayers = g_SharedConfig->maxPlayers;
        if (g_SharedConfig->maxZombies > 0)
            g_MaxZombies = g_SharedConfig->maxZombies;
        if (g_SharedConfig->maxAnimals > 0)
            g_MaxAnimals = g_SharedConfig->maxAnimals;
        if (g_SharedConfig->maxItems > 0)
            g_MaxItems = g_SharedConfig->maxItems;
        
        
        if (g_SharedConfig->colorPlayerBox != 0)
            ColorToFloat(g_SharedConfig->colorPlayerBox, g_ColorPlayerBox);
        if (g_SharedConfig->colorPlayerSkeleton != 0)
            ColorToFloat(g_SharedConfig->colorPlayerSkeleton, g_ColorPlayerSkeleton);
        if (g_SharedConfig->colorZombieBox != 0)
            ColorToFloat(g_SharedConfig->colorZombieBox, g_ColorZombieBox);
        if (g_SharedConfig->colorZombieSkeleton != 0)
            ColorToFloat(g_SharedConfig->colorZombieSkeleton, g_ColorZombieSkeleton);
        if (g_SharedConfig->colorAnimalBox != 0)
            ColorToFloat(g_SharedConfig->colorAnimalBox, g_ColorAnimalBox);
        if (g_SharedConfig->colorItemBox != 0)
            ColorToFloat(g_SharedConfig->colorItemBox, g_ColorItemBox);
        if (g_SharedConfig->colorVehicleBox != 0)
            ColorToFloat(g_SharedConfig->colorVehicleBox, g_ColorVehicleBox);
        
        
    }
}

static void DisconnectFromPanel()
{
    if (g_SharedConfig) {
        g_SharedConfig->dllAttached = false;
        UnmapViewOfFile(g_SharedConfig);
        g_SharedConfig = NULL;
    }
    if (g_hMapFile) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }
}

static void ZeroMem(void* dst, size_t size)
{
    volatile unsigned char* p = (volatile unsigned char*)dst;
    while (size--) *p++ = 0;
}

static bool IsValidPtr(uintptr_t addr)
{
    if (addr < 0x10000 || addr > 0x7FFFFFFFFFFF) return false;
    
    
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi)) == 0) return false;
    
    
    if (mbi.State != MEM_COMMIT) return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
    
    return true;
}


template<typename T>
static T Read(uintptr_t addr)
{
    T result;
    ZeroMem(&result, sizeof(T));
    
    if (addr < 0x10000 || addr > 0x7FFFFFFFFFFF) return result;
    
    
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi)) == 0) return result;
    if (mbi.State != MEM_COMMIT) return result;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return result;
    
    
    uintptr_t pageEnd = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    if (addr + sizeof(T) > pageEnd) return result;  
    
    
    __try {
        return *(T*)addr;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        
        return result;
    }
}


template<typename T>
static bool Write(uintptr_t addr, const T& value)
{
    if (addr < 0x10000 || addr > 0x7FFFFFFFFFFF) return false;
    
    
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi)) == 0) return false;
    if (mbi.State != MEM_COMMIT) return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
    
    
    
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
    
    uintptr_t pageEnd = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    if (addr + sizeof(T) > pageEnd) return false;
    
    
    __try {
        DWORD oldProtect;
        
        if (VirtualProtect((LPVOID)addr, sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            *(T*)addr = value;
            VirtualProtect((LPVOID)addr, sizeof(T), oldProtect, &oldProtect);
            return true;
        }
        return false;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

struct Vec3 { float x, y, z; };
struct Vec2 { float x, y; };


static float Sqrt(float x)
{
    if (x <= 0) return 0;
    float guess = x / 2.0f;
    for (int i = 0; i < 10; i++)
    {
        guess = (guess + x / guess) / 2.0f;
    }
    return guess;
}


static float Distance3D(const Vec3& a, const Vec3& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return Sqrt(dx * dx + dy * dy + dz * dz);
}


static Vec3 g_LocalPlayerPos = { 0, 0, 0 };
static bool g_LocalPlayerValid = false;
static Vec3 g_CameraPos = { 0, 0, 0 };
static bool g_CameraValid = false;


static void UpdateFastBullets()
{
    if (!g_FastBullets)
    {
        static int skipCount = 0;
        if (skipCount++ % 1000 == 0) 
        {
            char buf[128];
            wsprintfA(buf, "fb: g_FastBullets=%d g_GameModule=%p", g_FastBullets, g_GameModule);
            Log(buf);
        }
        return;
    }
    
    if (!g_GameModule)
    {
        Log("fb: game module null");
        return;
    }
    
    
    uintptr_t worldPtr = 0;
    uintptr_t worldOffsets[] = { 0xF4B0A0, 0xF4B050, 0xF4A0D0 };
    
    for (int w = 0; w < 3; w++)
    {
        uintptr_t test = Read<uintptr_t>((uintptr_t)g_GameModule + worldOffsets[w]);
        if (IsValidPtr(test) && test > 0x100000000)
        {
            uintptr_t camTest = Read<uintptr_t>(test + 0x1B8);
            if (IsValidPtr(camTest) && camTest > 0x100000000)
            {
                worldPtr = test;
                break;
            }
        }
    }
    
    if (!worldPtr)
    {
        static int failCount = 0;
        if (failCount++ % 1000 == 0)
        {
            Log("fb: no world ptr");
        }
        return;
    }
    
    
    
    
    uintptr_t localPlayer = 0;
    const char* playerMethod = "none";
    
    if (g_GameModule)
    {
        
        uintptr_t dayZPlayerPtr = Read<uintptr_t>((uintptr_t)g_GameModule + 0xF6D818);
        if (IsValidPtr(dayZPlayerPtr) && dayZPlayerPtr > 0x100000000)
        {
            localPlayer = Read<uintptr_t>(dayZPlayerPtr);
            if (IsValidPtr(localPlayer) && localPlayer > 0x100000000)
            {
                playerMethod = "base_address";
            }
            else
            {
                localPlayer = 0;
            }
        }
    }
    
    
    if (!localPlayer || !IsValidPtr(localPlayer) || localPlayer < 0x100000000)
    {
        localPlayer = Read<uintptr_t>(worldPtr + 0x2960);
        if (IsValidPtr(localPlayer) && localPlayer > 0x100000000)
        {
            playerMethod = "World::LocalPlayer";
        }
    }
    
    if (!IsValidPtr(localPlayer) || localPlayer < 0x100000000)
    {
        static int failCount = 0;
        if (failCount++ % 1000 == 0)
        {
            char buf[256];
            uintptr_t worldMethod = Read<uintptr_t>(worldPtr + 0x2960);
            uintptr_t baseMethod = 0;
            if (g_GameModule)
            {
                uintptr_t dayZPlayerPtr = Read<uintptr_t>((uintptr_t)g_GameModule + 0xF6D818);
                if (IsValidPtr(dayZPlayerPtr))
                    baseMethod = Read<uintptr_t>(dayZPlayerPtr);
            }
            wsprintfA(buf, "fb err: Invalid LocalPlayer (worldMethod=0x%p, baseMethod=0x%p)", 
                      (void*)worldMethod, (void*)baseMethod);
            Log(buf);
        }
        return;
    }
    
    
    uintptr_t playerInventory = 0;
    uintptr_t inventoryOffsets[] = { 0x510, 0x658 }; 
    
    for (int i = 0; i < 2; i++)
    {
        __try
        {
            playerInventory = Read<uintptr_t>(localPlayer + inventoryOffsets[i]);
            if (playerInventory != 0 && playerInventory >= 0x100000000 && playerInventory <= 0x7F0000000000 && IsValidPtr(playerInventory))
            {
                
                uintptr_t testHands = Read<uintptr_t>(playerInventory + 0xF8);
                if (!IsValidPtr(testHands) || testHands < 0x100000000)
                {
                    testHands = Read<uintptr_t>(playerInventory + 0x1B0);
                }
                if (IsValidPtr(testHands) && testHands >= 0x100000000)
                {
                    break; 
                }
            }
            playerInventory = 0;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            playerInventory = 0;
        }
    }
    
    
    
    static int cachedInventoryOffset = 0;
    static bool hasCachedOffset = false;
    
    if (playerInventory == 0)
    {
        
        if (hasCachedOffset && cachedInventoryOffset != 0)
        {
            __try
            {
                uintptr_t candidate = Read<uintptr_t>(localPlayer + cachedInventoryOffset);
                if (candidate != 0 && candidate >= 0x100000000 && candidate <= 0x7F0000000000 && IsValidPtr(candidate))
                {
                    uintptr_t handsAtF8 = Read<uintptr_t>(candidate + 0xF8);
                    uintptr_t handsAt1B0 = Read<uintptr_t>(candidate + 0x1B0);
                    if ((IsValidPtr(handsAtF8) && handsAtF8 >= 0x100000000) || 
                        (IsValidPtr(handsAt1B0) && handsAt1B0 >= 0x100000000))
                    {
                        playerInventory = candidate;
                    }
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                
                hasCachedOffset = false;
                cachedInventoryOffset = 0;
            }
        }
        
        
        static int bruteForceCount = 0;
        bruteForceCount++;
        
        if (playerInventory == 0 && bruteForceCount % 10 == 0)
        {
            
            for (int offset = -0x1000; offset <= 0x1000; offset += 8)
            {
                if (offset == 0) continue; 
                
                __try
                {
                    uintptr_t candidate = Read<uintptr_t>(localPlayer + 0x658 + offset);
                    if (candidate != 0 && candidate >= 0x100000000 && candidate <= 0x7F0000000000 && IsValidPtr(candidate))
                    {
                        
                        uintptr_t handsAtF8 = Read<uintptr_t>(candidate + 0xF8);
                        uintptr_t handsAt1B0 = Read<uintptr_t>(candidate + 0x1B0);
                        
                        if ((IsValidPtr(handsAtF8) && handsAtF8 >= 0x100000000) || 
                            (IsValidPtr(handsAt1B0) && handsAt1B0 >= 0x100000000))
                        {
                            playerInventory = candidate;
                            cachedInventoryOffset = 0x658 + offset;
                            hasCachedOffset = true;
                            char buf[256];
                            wsprintfA(buf, "fb: FOUND inventory via brute force: offset=0x%X (0x658+0x%X), ptr=0x%p", 
                                     0x658 + offset, offset, (void*)playerInventory);
                            Log(buf);
                            break;
                        }
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    continue;
                }
            }
        }
    }
    
    
    if (playerInventory == 0 || playerInventory < 0x100000000 || playerInventory > 0x7F0000000000)
    {
        static int failCount = 0;
        if (failCount++ % 1000 == 0)
        {
            char buf[512];
            
            DWORD invAsDWORD = 0;
            ULONG_PTR invAsULONG = 0;
            __try
            {
                invAsDWORD = Read<DWORD>(localPlayer + 0x658);
                invAsULONG = Read<ULONG_PTR>(localPlayer + 0x658);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                invAsDWORD = 0;
                invAsULONG = 0;
            }
            wsprintfA(buf, "fb err: Invalid PlayerInventory ptr=0x%p (localPlayer=0x%p, method=%s, invAsDWORD=0x%X, invAsULONG=0x%p)", 
                      (void*)playerInventory, (void*)localPlayer, playerMethod, invAsDWORD, (void*)invAsULONG);
            Log(buf);
        }
        return;
    }
    
    
    
    uintptr_t playerInventoryHands = Read<uintptr_t>(playerInventory + 0xF8);
    if (!IsValidPtr(playerInventoryHands) || playerInventoryHands < 0x100000000)
    {
        
        playerInventoryHands = Read<uintptr_t>(playerInventory + 0x1B0);
    }
    
    if (!IsValidPtr(playerInventoryHands) || playerInventoryHands < 0x100000000)
    {
        static int failCount = 0;
        if (failCount++ % 1000 == 0)
        {
            char buf[256];
            uintptr_t handsAtF8 = Read<uintptr_t>(playerInventory + 0xF8);
            uintptr_t handsAt1B0 = Read<uintptr_t>(playerInventory + 0x1B0);
            wsprintfA(buf, "fb err: Invalid PlayerInventoryHands (hands@0xF8=0x%p, hands@0x1B0=0x%p, inventory=0x%p)", 
                      (void*)handsAtF8, (void*)handsAt1B0, (void*)playerInventory);
            Log(buf);
        }
        return;
    }
    
    
    
    uintptr_t ammoType1 = 0;
    
    
    __try
    {
        ammoType1 = Read<uintptr_t>(playerInventoryHands + 0x6B0);
        if (IsValidPtr(ammoType1) && ammoType1 >= 0x100000000 && ammoType1 <= 0x7F0000000000)
        {
            
            uintptr_t testAmmoType2 = Read<uintptr_t>(ammoType1 + 0x20);
            if (!IsValidPtr(testAmmoType2) || testAmmoType2 < 0x100000000)
            {
                ammoType1 = 0; 
            }
        }
        else
        {
            ammoType1 = 0;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        ammoType1 = 0;
    }
    
    
    if (ammoType1 == 0)
    {
        __try
        {
            ammoType1 = Read<uintptr_t>(playerInventory + 0x6B0);
            if (IsValidPtr(ammoType1) && ammoType1 >= 0x100000000 && ammoType1 <= 0x7F0000000000)
            {
                uintptr_t testAmmoType2 = Read<uintptr_t>(ammoType1 + 0x20);
                if (!IsValidPtr(testAmmoType2) || testAmmoType2 < 0x100000000)
                {
                    ammoType1 = 0;
                }
            }
            else
            {
                ammoType1 = 0;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            ammoType1 = 0;
        }
    }
    
    
    
    static uintptr_t cachedAmmoType1 = 0;
    static int cachedAmmoType1Offset = 0;
    
    if (ammoType1 == 0)
    {
        if (cachedAmmoType1 != 0 && IsValidPtr(cachedAmmoType1))
        {
            
            ammoType1 = cachedAmmoType1;
        }
        else
        {
            
            static int bruteForceAmmoCount = 0;
            bruteForceAmmoCount++;
            
            
            
            int searchStart = (bruteForceAmmoCount % 20) * 0x100 - 0x1000; 
            int searchEnd = searchStart + 0x100;
            bool searchFromInventory = (bruteForceAmmoCount % 20) >= 10; 
            
            uintptr_t searchBase = searchFromInventory ? playerInventory : playerInventoryHands;
            
            for (int offset = searchStart; offset <= searchEnd; offset += 8)
            {
                if (offset == 0x6B0 && !searchFromInventory) continue; 
                
                __try
                {
                    uintptr_t candidate = Read<uintptr_t>(searchBase + offset);
                    if (candidate != 0 && candidate >= 0x100000000 && candidate <= 0x7F0000000000 && IsValidPtr(candidate))
                    {
                        
                        uintptr_t testAmmoType2 = Read<uintptr_t>(candidate + 0x20);
                        if (IsValidPtr(testAmmoType2) && testAmmoType2 >= 0x100000000 && testAmmoType2 <= 0x7F0000000000)
                        {
                            
                            __try
                            {
                                float testSpeed = Read<float>(testAmmoType2 + 0x364);
                                
                                if ((testSpeed > -1000.0f && testSpeed < 100000.0f) || true) 
                                {
                                    ammoType1 = candidate;
                                    cachedAmmoType1 = candidate;
                                    cachedAmmoType1Offset = offset;
                                    char buf[256];
                                    const char* baseName = searchFromInventory ? "inventory" : "hands";
                                    wsprintfA(buf, "fb: FOUND AmmoType1 via brute force: %s+0x%X, ptr=0x%p, ammoType2=0x%p, speed=%.2f", 
                                             baseName, offset, (void*)ammoType1, (void*)testAmmoType2, testSpeed);
                                    Log(buf);
                                    break;
                                }
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                
                                ammoType1 = candidate;
                                cachedAmmoType1 = candidate;
                                cachedAmmoType1Offset = offset;
                                char buf[256];
                                const char* baseName = searchFromInventory ? "inventory" : "hands";
                                wsprintfA(buf, "fb: FOUND AmmoType1 via brute force: %s+0x%X, ptr=0x%p, ammoType2=0x%p (speed read failed)", 
                                         baseName, offset, (void*)ammoType1, (void*)testAmmoType2);
                                Log(buf);
                                break;
                            }
                        }
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    continue;
                }
            }
        }
    }
    
    if (!IsValidPtr(ammoType1) || ammoType1 < 0x100000000)
    {
        static int failCount = 0;
        if (failCount++ % 1000 == 0)
        {
            char buf[256];
            wsprintfA(buf, "fb err: Invalid AmmoType1 ptr=0x%p (hands=0x%p)", 
                      (void*)ammoType1, (void*)playerInventoryHands);
            Log(buf);
        }
        return;
    }
    
    
    uintptr_t ammoType2 = Read<uintptr_t>(ammoType1 + 0x20);
    if (!IsValidPtr(ammoType2) || ammoType2 < 0x100000000)
    {
        static int failCount = 0;
        if (failCount++ % 1000 == 0)
        {
            char buf[128];
            wsprintfA(buf, "fb err: Invalid AmmoType2 ptr=0x%p (ammoType1=0x%p)", (void*)ammoType2, (void*)ammoType1);
            Log(buf);
        }
        return;
    }
    
    
    
    float distance = 1000.0f;
    
    
    float newSpeed = distance * 100.0f;
    
    
    
    
    uintptr_t potentialOffsets[] = { 0x364, 0x360, 0x368, 0x35C, 0x36C, 0x370, 0x354, 0x358, 0x380, 0x384 };
    int numOffsets = sizeof(potentialOffsets) / sizeof(potentialOffsets[0]);
    
    static int debugCount = 0;
    if (debugCount++ % 1000 == 0)
    {
        char buf[256];
        wsprintfA(buf, "fb: DEBUG: ammoType1=0x%p, ammoType2=0x%p", 
                  (void*)ammoType1, (void*)ammoType2);
        Log(buf);
    }
    
    
    
    for (int i = 0; i < numOffsets; i++)
    {
        uintptr_t initSpeedAddr = ammoType2 + potentialOffsets[i];
        
        if (IsValidPtr(initSpeedAddr) && initSpeedAddr > 0x100000000 && initSpeedAddr < 0x7F0000000000)
        {
            __try
            {
                
                float currentValue = Read<float>(initSpeedAddr);
                
                bool writeSuccess = Write<float>(initSpeedAddr, newSpeed);
                
                static int logCount = 0;
                if (logCount++ % 1000 == 0 && i == 0) 
                {
                    char buf[512];
                    int newSpeedInt = (int)newSpeed;
                    int currentValueInt = (int)currentValue;
                    wsprintfA(buf, "fb: Written speed=%d to AmmoType2+0x%X (was %d) addr=0x%p (success=%d)", 
                              newSpeedInt, potentialOffsets[i], currentValueInt, (void*)initSpeedAddr, writeSuccess);
                    Log(buf);
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                continue;
            }
        }
        
        
        uintptr_t initSpeedAddr1 = ammoType1 + potentialOffsets[i];
        if (IsValidPtr(initSpeedAddr1) && initSpeedAddr1 > 0x100000000 && initSpeedAddr1 < 0x7F0000000000)
        {
            __try
            {
                float currentValue = Read<float>(initSpeedAddr1);
                Write<float>(initSpeedAddr1, newSpeed);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                continue;
            }
        }
    }
    
    
    
    
    static int bulletListLogCount = 0;
    __try
    {
        uintptr_t bulletListPtr = Read<uintptr_t>(worldPtr + 0xE00);
        if (IsValidPtr(bulletListPtr) && bulletListPtr > 0x100000000)
        {
            
            int bulletCount = Read<int>(bulletListPtr + 0x8);
            if (bulletCount > 0 && bulletCount < 1000) 
            {
                
                uintptr_t bulletArray = Read<uintptr_t>(bulletListPtr);
                if (IsValidPtr(bulletArray) && bulletArray > 0x100000000)
                {
                    if (bulletListLogCount++ % 1000 == 0)
                    {
                        char buf[256];
                        wsprintfA(buf, "fb: BulletList: count=%d, array=0x%p", bulletCount, (void*)bulletArray);
                        Log(buf);
                    }
                    
                    
                    
                    static uintptr_t bulletSpeedOffsets[] = { 0x0, 0x4, 0x8, 0xC, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C };
                    static int numBulletOffsets = sizeof(bulletSpeedOffsets) / sizeof(bulletSpeedOffsets[0]);
                    
                    
                    int bulletsToCheck = (bulletCount < 5) ? bulletCount : 5;
                    for (int b = 0; b < bulletsToCheck; b++)
                    {
                        
                        uintptr_t bullet = bulletArray + (b * 0x100);
                        if (IsValidPtr(bullet) && bullet > 0x100000000)
                        {
                            
                            for (int o = 0; o < numBulletOffsets; o++)
                            {
                                uintptr_t speedAddr = bullet + bulletSpeedOffsets[o];
                                if (IsValidPtr(speedAddr) && speedAddr > 0x100000000)
                                {
                                    float testValue = Read<float>(speedAddr);
                                    
                                    if (testValue > 0.0f && testValue < 2000.0f)
                                    {
                                        Write<float>(speedAddr, newSpeed);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        
    }
}


static bool GetEntityPosition(uintptr_t entity, Vec3& outPos);


static void UpdateBulletAimbot()
{
    if (!g_FastBullets) return; 
    
    if (!g_GameModule) return;
    
    
    uintptr_t worldPtr = 0;
    uintptr_t worldOffsets[] = { 0xF4B0A0, 0xF4B050, 0xF4A0D0 };
    
    for (int w = 0; w < 3; w++)
    {
        uintptr_t test = Read<uintptr_t>((uintptr_t)g_GameModule + worldOffsets[w]);
        if (IsValidPtr(test) && test > 0x100000000)
        {
            uintptr_t camTest = Read<uintptr_t>(test + 0x1B8);
            if (IsValidPtr(camTest) && camTest > 0x100000000)
            {
                worldPtr = test;
                break;
            }
        }
    }
    
    if (!worldPtr) return;
    
    
    uintptr_t localPlayer = 0;
    if (g_GameModule)
    {
        uintptr_t dayZPlayerPtr = Read<uintptr_t>((uintptr_t)g_GameModule + 0xF6D818);
        if (IsValidPtr(dayZPlayerPtr) && dayZPlayerPtr > 0x100000000)
        {
            localPlayer = Read<uintptr_t>(dayZPlayerPtr);
            if (!IsValidPtr(localPlayer) || localPlayer < 0x100000000)
            {
                localPlayer = Read<uintptr_t>(worldPtr + 0x2960);
            }
        }
        else
        {
            localPlayer = Read<uintptr_t>(worldPtr + 0x2960);
        }
    }
    
    if (!IsValidPtr(localPlayer) || localPlayer < 0x100000000) return;
    
    
    Vec3 localPos;
    if (!GetEntityPosition(localPlayer, localPos)) return;
    
    
    uintptr_t closestEntity = 0;
    float closestDist = 10000.0f;
    Vec3 targetPos;
    
    static int aimbotLogCount = 0;
    int entitiesChecked = 0;
    
    
    for (int listOffset = 0; listOffset < 2; listOffset++)
    {
        uintptr_t offset = (listOffset == 0) ? 0xF48 : 0x1090;
        uintptr_t listAddr = worldPtr + offset;
        uintptr_t entListPtr = Read<uintptr_t>(listAddr);
        
        if (!IsValidPtr(entListPtr) || entListPtr < 0x100000000) continue;
        
        int entCount = Read<int>(listAddr + 0x8);
        if (entCount <= 0 || entCount > 1000) continue;
        
        for (int i = 0; i < entCount && i < 100; i++)
        {
            uintptr_t entityAddr = entListPtr + (i * 0x8);
            if (!IsValidPtr(entityAddr)) continue;
            
            uintptr_t entity = Read<uintptr_t>(entityAddr);
            if (!IsValidPtr(entity) || entity < 0x100000000) continue;
            if (entity == localPlayer) continue;
            
            entitiesChecked++;
            
            
            uintptr_t entityType = Read<uintptr_t>(entity + 0x180);
            if (!IsValidPtr(entityType) || entityType < 0x100000000) continue;
            
            
            Vec3 entPos;
            if (!GetEntityPosition(entity, entPos)) continue;
            
            
            float dist = Distance3D(localPos, entPos);
            if (dist < closestDist && dist > 0.1f && dist < 500.0f)
            {
                closestDist = dist;
                closestEntity = entity;
                targetPos = entPos;
            }
        }
    }
    
    if (aimbotLogCount++ % 1000 == 0)
    {
        char buf[256];
        wsprintfA(buf, "aim: Checked %d entities, closest=0x%p dist=%.1f", entitiesChecked, (void*)closestEntity, closestDist);
        Log(buf);
    }
    
    if (!closestEntity) return;
    
    
    Vec3 direction;
    direction.x = targetPos.x - localPos.x;
    direction.y = targetPos.y - localPos.y;
    direction.z = targetPos.z - localPos.z;
    
    
    float length = Distance3D({0,0,0}, direction);
    if (length < 0.001f) return;
    
    direction.x /= length;
    direction.y /= length;
    direction.z /= length;
    
    
    float bulletSpeed = 1000.0f;
    direction.x *= bulletSpeed;
    direction.y *= bulletSpeed;
    direction.z *= bulletSpeed;
    
    
    static int bulletModifyCount = 0;
    __try
    {
        uintptr_t bulletListPtr = Read<uintptr_t>(worldPtr + 0xE00);
        if (IsValidPtr(bulletListPtr) && bulletListPtr > 0x100000000)
        {
            int bulletCount = Read<int>(bulletListPtr + 0x8);
            if (bulletCount > 0 && bulletCount < 100)
            {
                uintptr_t bulletArray = Read<uintptr_t>(bulletListPtr);
                if (IsValidPtr(bulletArray) && bulletArray > 0x100000000)
                {
                    if (bulletModifyCount++ % 100 == 0)
                    {
                        char buf[256];
                        wsprintfA(buf, "aim: Modifying %d bullets, target dist=%.1f", bulletCount, closestDist);
                        Log(buf);
                    }
                    
                    
                    
                    for (int b = 0; b < bulletCount && b < 10; b++)
                    {
                        uintptr_t bullet = bulletArray + (b * 0x100);
                        if (IsValidPtr(bullet) && bullet > 0x100000000)
                        {
                            
                            uintptr_t velOffsets[] = { 0x0, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0 };
                            for (int vo = 0; vo < 12; vo++)
                            {
                                uintptr_t velX = bullet + velOffsets[vo];
                                uintptr_t velY = velX + 4;
                                uintptr_t velZ = velX + 8;
                                
                                if (IsValidPtr(velX) && IsValidPtr(velY) && IsValidPtr(velZ))
                                {
                                    
                                    Write<float>(velX, direction.x);
                                    Write<float>(velY, direction.y);
                                    Write<float>(velZ, direction.z);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        
    }
}


static void AddLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a);


struct CharDef {
    unsigned char numSegments;
    unsigned char segments[20];  
};

static const CharDef g_Font[95] = {
    
    { 0, {} },
    
    { 2, { 2,0, 2,4,  2,6, 2,6 } },
    
    { 2, { 1,0, 1,2,  3,0, 3,2 } },
    
    { 4, { 1,1, 1,5,  3,1, 3,5,  0,2, 4,2,  0,4, 4,4 } },
    
    { 4, { 2,0, 2,6,  4,1, 0,1,  0,3, 4,3,  0,5, 4,5 } },
    
    { 3, { 0,0, 1,1,  4,0, 0,6,  3,5, 4,6 } },
    
    { 5, { 4,6, 0,2,  0,2, 2,0,  2,0, 4,2,  4,2, 0,6,  0,6, 2,6 } },
    
    { 1, { 2,0, 2,2 } },
    
    { 3, { 3,0, 1,2,  1,2, 1,4,  1,4, 3,6 } },
    
    { 3, { 1,0, 3,2,  3,2, 3,4,  3,4, 1,6 } },
    
    { 3, { 2,1, 2,5,  0,2, 4,4,  4,2, 0,4 } },
    
    { 2, { 2,1, 2,5,  0,3, 4,3 } },
    
    { 1, { 2,5, 1,7 } },
    
    { 1, { 0,3, 4,3 } },
    
    { 1, { 2,5, 2,6 } },
    
    { 1, { 0,6, 4,0 } },
    
    { 4, { 0,0, 4,0,  4,0, 4,6,  4,6, 0,6,  0,6, 0,0 } },
    
    { 3, { 1,1, 2,0,  2,0, 2,6,  1,6, 3,6 } },
    
    { 5, { 0,1, 2,0,  2,0, 4,1,  4,1, 0,6,  0,6, 4,6 } },
    
    { 5, { 0,0, 4,0,  4,0, 4,6,  4,6, 0,6,  0,3, 4,3 } },
    
    { 3, { 0,0, 0,3,  0,3, 4,3,  4,0, 4,6 } },
    
    { 5, { 4,0, 0,0,  0,0, 0,3,  0,3, 4,3,  4,3, 4,6,  4,6, 0,6 } },
    
    { 5, { 4,0, 0,0,  0,0, 0,6,  0,6, 4,6,  4,6, 4,3,  4,3, 0,3 } },
    
    { 2, { 0,0, 4,0,  4,0, 2,6 } },
    
    { 5, { 0,0, 4,0,  4,0, 4,6,  4,6, 0,6,  0,6, 0,0,  0,3, 4,3 } },
    
    { 5, { 4,3, 0,3,  0,3, 0,0,  0,0, 4,0,  4,0, 4,6,  4,6, 0,6 } },
    
    { 2, { 2,2, 2,2,  2,5, 2,5 } },
    
    { 2, { 2,2, 2,2,  2,5, 1,6 } },
    
    { 2, { 4,0, 0,3,  0,3, 4,6 } },
    
    { 2, { 0,2, 4,2,  0,4, 4,4 } },
    
    { 2, { 0,0, 4,3,  4,3, 0,6 } },
    
    { 4, { 0,1, 2,0,  2,0, 4,1,  4,1, 2,4,  2,6, 2,6 } },
    
    { 5, { 4,2, 2,2,  2,2, 2,4,  2,4, 4,4,  0,0, 4,0,  4,0, 0,6 } },
    
    { 3, { 0,6, 2,0,  2,0, 4,6,  0,4, 4,4 } },
    
    { 5, { 0,0, 0,6,  0,0, 3,0,  3,0, 0,3,  0,3, 3,3,  3,3, 0,6 } },
    
    { 3, { 4,0, 0,0,  0,0, 0,6,  0,6, 4,6 } },
    
    { 4, { 0,0, 0,6,  0,0, 3,0,  3,0, 4,3,  4,3, 0,6 } },
    
    { 4, { 4,0, 0,0,  0,0, 0,6,  0,6, 4,6,  0,3, 3,3 } },
    
    { 3, { 4,0, 0,0,  0,0, 0,6,  0,3, 3,3 } },
    
    { 4, { 4,0, 0,0,  0,0, 0,6,  0,6, 4,6,  4,6, 4,3 } },
    
    { 3, { 0,0, 0,6,  4,0, 4,6,  0,3, 4,3 } },
    
    { 3, { 1,0, 3,0,  2,0, 2,6,  1,6, 3,6 } },
    
    { 3, { 0,0, 4,0,  4,0, 4,6,  4,6, 0,6 } },
    
    { 3, { 0,0, 0,6,  4,0, 0,3,  0,3, 4,6 } },
    
    { 2, { 0,0, 0,6,  0,6, 4,6 } },
    
    { 4, { 0,6, 0,0,  0,0, 2,3,  2,3, 4,0,  4,0, 4,6 } },
    
    { 3, { 0,6, 0,0,  0,0, 4,6,  4,6, 4,0 } },
    
    { 4, { 0,0, 4,0,  4,0, 4,6,  4,6, 0,6,  0,6, 0,0 } },
    
    { 4, { 0,6, 0,0,  0,0, 4,0,  4,0, 4,3,  4,3, 0,3 } },
    
    { 5, { 0,0, 4,0,  4,0, 4,4,  4,4, 0,6,  0,6, 0,0,  2,4, 4,6 } },
    
    { 5, { 0,6, 0,0,  0,0, 4,0,  4,0, 4,3,  4,3, 0,3,  0,3, 4,6 } },
    
    { 5, { 4,0, 0,0,  0,0, 0,3,  0,3, 4,3,  4,3, 4,6,  4,6, 0,6 } },
    
    { 2, { 0,0, 4,0,  2,0, 2,6 } },
    
    { 3, { 0,0, 0,6,  0,6, 4,6,  4,6, 4,0 } },
    
    { 2, { 0,0, 2,6,  2,6, 4,0 } },
    
    { 4, { 0,0, 0,6,  0,6, 2,3,  2,3, 4,6,  4,6, 4,0 } },
    
    { 2, { 0,0, 4,6,  4,0, 0,6 } },
    
    { 3, { 0,0, 2,3,  4,0, 2,3,  2,3, 2,6 } },
    
    { 3, { 0,0, 4,0,  4,0, 0,6,  0,6, 4,6 } },
    
    { 3, { 3,0, 1,0,  1,0, 1,6,  1,6, 3,6 } },
    
    { 1, { 0,0, 4,6 } },
    
    { 3, { 1,0, 3,0,  3,0, 3,6,  3,6, 1,6 } },
    
    { 2, { 0,2, 2,0,  2,0, 4,2 } },
    
    { 1, { 0,6, 4,6 } },
    
    { 1, { 1,0, 2,1 } },
    
    { 4, { 0,2, 4,2,  4,2, 4,6,  4,6, 0,6,  0,6, 0,4 } },
    
    { 4, { 0,0, 0,6,  0,6, 4,6,  4,6, 4,3,  4,3, 0,3 } },
    
    { 3, { 4,2, 0,2,  0,2, 0,6,  0,6, 4,6 } },
    
    { 4, { 4,0, 4,6,  4,6, 0,6,  0,6, 0,3,  0,3, 4,3 } },
    
    { 4, { 0,4, 4,4,  4,4, 4,2,  0,2, 0,6,  0,6, 4,6 } },
    
    { 3, { 3,0, 1,0,  1,0, 1,6,  0,3, 3,3 } },
    
    { 5, { 4,2, 0,2,  0,2, 0,5,  0,5, 4,5,  4,2, 4,8,  4,8, 0,8 } },
    
    { 3, { 0,0, 0,6,  0,3, 4,3,  4,3, 4,6 } },
    
    { 2, { 2,0, 2,1,  2,3, 2,6 } },
    
    { 3, { 3,0, 3,1,  3,3, 3,7,  3,7, 0,7 } },
    
    { 3, { 0,0, 0,6,  4,2, 0,4,  0,4, 4,6 } },
    
    { 2, { 2,0, 2,5,  2,5, 3,6 } },
    
    { 4, { 0,2, 0,6,  0,2, 2,2,  2,2, 2,6,  2,2, 4,2 } },
    
    { 3, { 0,2, 0,6,  0,2, 4,2,  4,2, 4,6 } },
    
    { 4, { 0,2, 4,2,  4,2, 4,6,  4,6, 0,6,  0,6, 0,2 } },
    
    { 4, { 0,2, 0,8,  0,2, 4,2,  4,2, 4,5,  4,5, 0,5 } },
    
    { 4, { 4,2, 4,8,  4,2, 0,2,  0,2, 0,5,  0,5, 4,5 } },
    
    { 2, { 0,2, 0,6,  0,2, 4,2 } },
    
    { 4, { 4,2, 0,2,  0,2, 0,4,  0,4, 4,6,  4,6, 0,6 } },
    
    { 3, { 1,0, 1,5,  1,5, 3,6,  0,2, 3,2 } },
    
    { 3, { 0,2, 0,6,  0,6, 4,6,  4,2, 4,6 } },
    
    { 2, { 0,2, 2,6,  2,6, 4,2 } },
    
    { 4, { 0,2, 0,6,  0,6, 2,4,  2,4, 4,6,  4,6, 4,2 } },
    
    { 2, { 0,2, 4,6,  4,2, 0,6 } },
    
    { 4, { 0,2, 0,5,  0,5, 4,5,  4,2, 4,8,  4,8, 0,8 } },
    
    { 3, { 0,2, 4,2,  4,2, 0,6,  0,6, 4,6 } },
    
    { 4, { 3,0, 2,1,  2,1, 2,5,  2,5, 3,6,  1,3, 2,3 } },
    
    { 1, { 2,0, 2,6 } },
    
    { 4, { 1,0, 2,1,  2,1, 2,5,  2,5, 1,6,  2,3, 3,3 } },
    
    { 2, { 0,3, 2,2,  2,2, 4,3 } },
};


static float DrawChar(float x, float y, char c, float scale, float r, float g, float b, float a)
{
    if (c < 32 || c > 126) return scale * 4;  
    
    const CharDef& def = g_Font[c - 32];
    
    for (int i = 0; i < def.numSegments && i < 5; i++)
    {
        unsigned char x1 = def.segments[i * 4 + 0];
        unsigned char y1 = def.segments[i * 4 + 1];
        unsigned char x2 = def.segments[i * 4 + 2];
        unsigned char y2 = def.segments[i * 4 + 3];
        
        
        float px1 = x + x1 * scale;
        float py1 = y + y1 * scale;
        float px2 = x + x2 * scale;
        float py2 = y + y2 * scale;
        
        
        AddLine(px1, py1, px2, py2, r, g, b, a);
    }
    
    return scale * 6;  
}


static void DrawTextD3D(float x, float y, const char* text, float r, float g, float b, float a, float scale)
{
    if (!text) return;
    
    float curX = x;
    
    
    float shadowX = x + 1;
    for (int i = 0; text[i]; i++)
    {
        shadowX += DrawChar(shadowX, y + 1, text[i], scale, 0.0f, 0.0f, 0.0f, a * 0.8f);
    }
    
    
    for (int i = 0; text[i]; i++)
    {
        curX += DrawChar(curX, y, text[i], scale, r, g, b, a);
    }
}


static void IntToStr(int value, char* buf)
{
    if (value == 0) { buf[0] = '0'; buf[1] = 0; return; }
    
    char temp[16];
    int i = 0;
    bool neg = false;
    
    if (value < 0) { neg = true; value = -value; }
    
    while (value > 0 && i < 15)
    {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    int j = 0;
    if (neg) buf[j++] = '-';
    while (i > 0) buf[j++] = temp[--i];
    buf[j] = 0;
}


struct Vertex {
    float x, y;
    float r, g, b, a;
};

#define MAX_VERTICES 65536
static Vertex g_Vertices[MAX_VERTICES];
static int g_VertexCount = 0;

static void InitRenderer()
{
    if (!g_Device || !g_Context) return;
    
    Log("making shaders...");
    
    
    HRESULT hr = g_Device->CreateVertexShader(g_VS, sizeof(g_VS), NULL, &g_VertexShader);
    if (FAILED(hr)) { 
        char buf[64]; 
        wsprintfA(buf, "vs failed 0x%x", hr); 
        Log(buf); 
        return; 
    }
    
    
    hr = g_Device->CreatePixelShader(g_PS, sizeof(g_PS), NULL, &g_PixelShader);
    if (FAILED(hr)) { 
        char buf[64]; 
        wsprintfA(buf, "ps failed 0x%x", hr); 
        Log(buf); 
        return; 
    }
    
    
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = g_Device->CreateInputLayout(layout, 2, g_VS, sizeof(g_VS), &g_InputLayout);
    if (FAILED(hr)) { 
        char buf[64]; 
        wsprintfA(buf, "il failed 0x%x", hr); 
        Log(buf); 
        return; 
    }
    
    
    D3D11_BUFFER_DESC bd;
    ZeroMem(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(Vertex) * MAX_VERTICES;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = g_Device->CreateBuffer(&bd, NULL, &g_VertexBuffer);
    if (FAILED(hr)) { 
        char buf[64]; 
        wsprintfA(buf, "vb failed 0x%x", hr); 
        Log(buf); 
        return; 
    }
    
    
    D3D11_BLEND_DESC blendDesc;
    ZeroMem(&blendDesc, sizeof(blendDesc));
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    g_Device->CreateBlendState(&blendDesc, &g_BlendState);
    
    
    D3D11_RASTERIZER_DESC rastDesc;
    ZeroMem(&rastDesc, sizeof(rastDesc));
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_NONE;
    rastDesc.DepthClipEnable = TRUE;
    g_Device->CreateRasterizerState(&rastDesc, &g_RasterizerState);
    
    
    D3D11_DEPTH_STENCIL_DESC dsDesc;
    ZeroMem(&dsDesc, sizeof(dsDesc));
    dsDesc.DepthEnable = FALSE;
    dsDesc.StencilEnable = FALSE;
    g_Device->CreateDepthStencilState(&dsDesc, &g_DepthStencilState);
    
    g_RenderInit = true;
    Log("renderer ok");
}

static void BeginDraw()
{
    g_VertexCount = 0;
}


static void AddLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a)
{
    if (g_VertexCount + 2 > MAX_VERTICES) return;
    
    
    float nx1 = x1 * g_NDCScaleX - 1.0f;
    float ny1 = 1.0f - y1 * g_NDCScaleY;
    float nx2 = x2 * g_NDCScaleX - 1.0f;
    float ny2 = 1.0f - y2 * g_NDCScaleY;
    
    g_Vertices[g_VertexCount++] = { nx1, ny1, r, g, b, a };
    g_Vertices[g_VertexCount++] = { nx2, ny2, r, g, b, a };
}


static void AddBox(float x, float y, float w, float h, float r, float g, float b, float a)
{
    if (g_VertexCount + 8 > MAX_VERTICES) return;
    
    
    float x2 = x + w;
    float y2 = y + h;
    
    float nx1 = x * g_NDCScaleX - 1.0f;
    float ny1 = 1.0f - y * g_NDCScaleY;
    float nx2 = x2 * g_NDCScaleX - 1.0f;
    float ny2 = 1.0f - y2 * g_NDCScaleY;
    
    
    g_Vertices[g_VertexCount++] = { nx1, ny1, r, g, b, a };
    g_Vertices[g_VertexCount++] = { nx2, ny1, r, g, b, a };
    
    g_Vertices[g_VertexCount++] = { nx2, ny1, r, g, b, a };
    g_Vertices[g_VertexCount++] = { nx2, ny2, r, g, b, a };
    
    g_Vertices[g_VertexCount++] = { nx2, ny2, r, g, b, a };
    g_Vertices[g_VertexCount++] = { nx1, ny2, r, g, b, a };
    
    g_Vertices[g_VertexCount++] = { nx1, ny2, r, g, b, a };
    g_Vertices[g_VertexCount++] = { nx1, ny1, r, g, b, a };
}

static void EndDraw()
{
    if (g_VertexCount == 0 || !g_RenderInit) return;
    
    
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(g_Context->Map(g_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        for (int i = 0; i < g_VertexCount; i++)
            ((Vertex*)mapped.pData)[i] = g_Vertices[i];
        g_Context->Unmap(g_VertexBuffer, 0);
    }
    
    
    g_Context->OMSetRenderTargets(1, &g_RenderTarget, NULL);
    g_Context->OMSetBlendState(g_BlendState, NULL, 0xFFFFFFFF);
    g_Context->OMSetDepthStencilState(g_DepthStencilState, 0);
    g_Context->RSSetState(g_RasterizerState);
    
    
    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = g_ScreenWidth;
    vp.Height = g_ScreenHeight;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    g_Context->RSSetViewports(1, &vp);
    
    
    g_Context->VSSetShader(g_VertexShader, NULL, 0);
    g_Context->PSSetShader(g_PixelShader, NULL, 0);
    g_Context->IASetInputLayout(g_InputLayout);
    
    
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_Context->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
    g_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    
    
    g_Context->Draw(g_VertexCount, 0);
}


static bool WorldToScreen(Vec3 world, Vec2& screen)
{
    if (!g_GameModule) return false;
    
    
    uintptr_t worldPtr = 0;
    uintptr_t worldOffsets[] = { 0xF4B050, 0xF4A0D0, 0xF4B0A0 };  
    for (int i = 0; i < 3; i++)
    {
        uintptr_t test = Read<uintptr_t>((uintptr_t)g_GameModule + worldOffsets[i]);
        if (IsValidPtr(test) && test > 0x100000000 && test < 0x7F0000000000)
        {
            worldPtr = test;
            break;
        }
    }
    if (!IsValidPtr(worldPtr) || worldPtr < 0x100000000) return false;
    
    uintptr_t camera = Read<uintptr_t>(worldPtr + offsets::world::Camera);
    if (!IsValidPtr(camera) || camera < 0x100000000) return false;
    
    
    Vec3 invRight = Read<Vec3>(camera + offsets::camera::InvertedViewRight);
    Vec3 invUp = Read<Vec3>(camera + offsets::camera::InvertedViewUp);
    Vec3 invForward = Read<Vec3>(camera + offsets::camera::InvertedViewForward);
    Vec3 invTranslation = Read<Vec3>(camera + offsets::camera::InvertedViewTranslation);
    Vec2 viewPortSize = Read<Vec2>(camera + offsets::camera::ViewPortSize);  
    Vec3 projD1 = Read<Vec3>(camera + offsets::camera::GetProjectionD1);
    Vec3 projD2 = Read<Vec3>(camera + offsets::camera::GetProjectionD2);
    
    
    Vec3 temp;
    temp.x = world.x - invTranslation.x;
    temp.y = world.y - invTranslation.y;
    temp.z = world.z - invTranslation.z;
    
    
    float x = temp.x * invRight.x + temp.y * invRight.y + temp.z * invRight.z;
    float y = temp.x * invUp.x + temp.y * invUp.y + temp.z * invUp.z;
    float z = temp.x * invForward.x + temp.y * invForward.y + temp.z * invForward.z;
    
    if (z < 0.65f) return false;  
    
    
    if (projD1.x == 0.0f || projD2.y == 0.0f) return false;
    
    
    float invZ = 1.0f / z;  
    float normalizedX = (x / projD1.x) * invZ;
    float normalizedY = (y / projD2.y) * invZ;
    
    
    screen.x = g_ScreenHalfW + (normalizedX * g_ScreenHalfW);
    screen.y = g_ScreenHalfH - (normalizedY * g_ScreenHalfH);
    
    return screen.x >= 0 && screen.x <= g_ScreenWidth && screen.y >= 0 && screen.y <= g_ScreenHeight;
}


struct Matrix3x4 {
    float m[12];  
};


enum BoneID {
    BONE_HEAD = 0,
    BONE_NECK = 1,
    BONE_SPINE2 = 2,
    BONE_SPINE1 = 3,
    BONE_PELVIS = 4,
    BONE_L_SHOULDER = 5,
    BONE_R_SHOULDER = 6,
    BONE_L_ELBOW = 7,
    BONE_R_ELBOW = 8,
    BONE_L_HAND = 9,
    BONE_R_HAND = 10,
    BONE_L_HIP = 11,
    BONE_R_HIP = 12,
    BONE_L_KNEE = 13,
    BONE_R_KNEE = 14,
    BONE_L_FOOT = 15,
    BONE_R_FOOT = 16,
    BONES_MAX = 17
};


static int g_PlayerBones[BONES_MAX] = { 24, 21, 20, 19, 0, 61, 94, 63, 97, 65, 99, 1, 9, 4, 12, 6, 14 };


static int g_ZombieBones[BONES_MAX] = { 21, 19, 16, 15, 0, 24, 56, 25, 59, 27, 60, 1, 9, 4, 12, 6, 13 };


struct SkeletonCache
{
    bool valid;                 
    bool isPlayer;              
    Matrix3x4 playerMatrix;     
    uintptr_t matrixClass;      
};


static bool InitSkeletonCache(uintptr_t entity, bool isPlayer, SkeletonCache& cache)
{
    cache.valid = false;
    cache.isPlayer = isPlayer;
    
    if (!IsValidPtr(entity) || entity < 0x100000000) return false;
    
    
    uintptr_t visualState = Read<uintptr_t>(entity + 0x1C8);  
    if (!IsValidPtr(visualState) || visualState < 0x100000000) return false;
    
    
    for (int i = 0; i < 12; i++)
    {
        cache.playerMatrix.m[i] = Read<float>(visualState + 0x8 + i * 4);
    }
    
    
    uintptr_t skeleton = Read<uintptr_t>(entity + (isPlayer ? 0x7E8 : 0x678));
    if (!IsValidPtr(skeleton) || skeleton < 0x100000000) return false;
    
    
    uintptr_t animClass = Read<uintptr_t>(skeleton + 0xB0);  
    if (!IsValidPtr(animClass) || animClass < 0x100000000) return false;
    
    
    cache.matrixClass = Read<uintptr_t>(animClass + 0xBF0);  
    if (!IsValidPtr(cache.matrixClass) || cache.matrixClass < 0x100000000) return false;
    
    cache.valid = true;
    return true;
}


static Vec3 GetBonePositionCached(const SkeletonCache& cache, int boneIndex)
{
    Vec3 result = { 0, 0, 0 };
    
    if (!cache.valid) return result;
    if (boneIndex < 0 || boneIndex >= BONES_MAX) return result;
    
    
    int boneId = cache.isPlayer ? g_PlayerBones[boneIndex] : g_ZombieBones[boneIndex];
    
    
    uintptr_t boneAddr = cache.matrixClass + 0x54 + (boneId * 48);
    
    Vec3 boneLocal;
    boneLocal.x = Read<float>(boneAddr);
    boneLocal.y = Read<float>(boneAddr + 4);
    boneLocal.z = Read<float>(boneAddr + 8);
    
    
    const float* m = cache.playerMatrix.m;
    result.x = (m[0] * boneLocal.x) + (m[3] * boneLocal.y) + (m[6] * boneLocal.z) + m[9];
    result.y = (m[1] * boneLocal.x) + (m[4] * boneLocal.y) + (m[7] * boneLocal.z) + m[10];
    result.z = (m[2] * boneLocal.x) + (m[5] * boneLocal.y) + (m[8] * boneLocal.z) + m[11];
    
    return result;
}


static Vec3 GetBonePosition(uintptr_t entity, int boneIndex, bool isPlayer)
{
    SkeletonCache cache;
    if (!InitSkeletonCache(entity, isPlayer, cache))
    {
        Vec3 zero = { 0, 0, 0 };
        return zero;
    }
    return GetBonePositionCached(cache, boneIndex);
}


static void DrawEntitySkeleton(uintptr_t entity, bool isPlayer, const char* name, int distance)
{
    
    
    SkeletonCache cache;
    if (!InitSkeletonCache(entity, isPlayer, cache))
    {
        return;  
    }
    
    
    Vec3 bonePositions[BONES_MAX];
    Vec2 boneScreenPositions[BONES_MAX];
    bool boneValid[BONES_MAX];
    
    for (int i = 0; i < BONES_MAX; i++)
    {
        
        bonePositions[i] = GetBonePositionCached(cache, i);
        boneValid[i] = WorldToScreen(bonePositions[i], boneScreenPositions[i]);
    }
    
    
    if (!boneValid[BONE_HEAD] || !boneValid[BONE_PELVIS]) return;
    
    
    Vec2 headScreen = boneScreenPositions[BONE_HEAD];
    Vec2 leftFootScreen = boneScreenPositions[BONE_L_FOOT];
    Vec2 rightFootScreen = boneScreenPositions[BONE_R_FOOT];
    
    
    float footY = leftFootScreen.y;
    if (boneValid[BONE_R_FOOT] && rightFootScreen.y > footY) footY = rightFootScreen.y;
    if (!boneValid[BONE_L_FOOT] && boneValid[BONE_R_FOOT]) footY = rightFootScreen.y;
    
    float height = footY - headScreen.y;
    if (height < 2.0f) return;  
    
    float width = height / 2.5f;
    float x = headScreen.x - width / 2.0f;
    float y = headScreen.y;
    
    
    if (isPlayer && g_PlayerBox)
        AddBox(x, y, width, height, g_ColorPlayerBox[0], g_ColorPlayerBox[1], g_ColorPlayerBox[2], g_ColorPlayerBox[3]);
    else if (!isPlayer && g_ZombieBox)
        AddBox(x, y, width, height, g_ColorZombieBox[0], g_ColorZombieBox[1], g_ColorZombieBox[2], g_ColorZombieBox[3]);
    
    
    static int skeletonBones[][2] = {
        {BONE_NECK, BONE_SPINE2},
        {BONE_SPINE2, BONE_SPINE1},
        {BONE_SPINE1, BONE_PELVIS},
        {BONE_PELVIS, BONE_L_HIP},
        {BONE_L_HIP, BONE_L_KNEE},
        {BONE_L_KNEE, BONE_L_FOOT},
        {BONE_PELVIS, BONE_R_HIP},
        {BONE_R_HIP, BONE_R_KNEE},
        {BONE_R_KNEE, BONE_R_FOOT},
        {BONE_NECK, BONE_L_SHOULDER},
        {BONE_L_SHOULDER, BONE_L_ELBOW},
        {BONE_L_ELBOW, BONE_L_HAND},
        {BONE_NECK, BONE_R_SHOULDER},
        {BONE_R_SHOULDER, BONE_R_ELBOW},
        {BONE_R_ELBOW, BONE_R_HAND}
    };
    
    
    for (int i = 0; i < 15; i++)
    {
        int from = skeletonBones[i][0];
        int to = skeletonBones[i][1];
        
        if (boneValid[from] && boneValid[to])
        {
            if (isPlayer)
                AddLine(boneScreenPositions[from].x, boneScreenPositions[from].y,
                       boneScreenPositions[to].x, boneScreenPositions[to].y,
                       g_ColorPlayerSkeleton[0], g_ColorPlayerSkeleton[1], g_ColorPlayerSkeleton[2], g_ColorPlayerSkeleton[3]);
            else
                AddLine(boneScreenPositions[from].x, boneScreenPositions[from].y,
                       boneScreenPositions[to].x, boneScreenPositions[to].y,
                       g_ColorZombieSkeleton[0], g_ColorZombieSkeleton[1], g_ColorZombieSkeleton[2], g_ColorZombieSkeleton[3]);
        }
    }
    
    
        if (boneValid[BONE_HEAD])
        {
            float headSize = height / 8.0f;
            if (headSize < 3.0f) headSize = 3.0f;
            if (headSize > 15.0f) headSize = 15.0f;
            
            
            if (isPlayer && g_PlayerBox)
                AddBox(headScreen.x - headSize/2, headScreen.y - headSize/2, headSize, headSize, 
                   g_ColorPlayerBox[0], g_ColorPlayerBox[1], g_ColorPlayerBox[2], g_ColorPlayerBox[3]);
            else if (!isPlayer && g_ZombieBox)
                AddBox(headScreen.x - headSize/2, headScreen.y - headSize/2, headSize, headSize,
                   g_ColorZombieBox[0], g_ColorZombieBox[1], g_ColorZombieBox[2], g_ColorZombieBox[3]);
        }
    
    
    bool showName = (isPlayer && g_PlayerName) || (!isPlayer && g_ZombieName);
    bool showDist = (isPlayer && g_PlayerDistanceEnabled) || (!isPlayer && g_ZombieDistanceEnabled);
    
    if (showName || showDist)
    {
        
        
        float textScale = 1.8f;  
        float charWidth = textScale * 6.0f;  
        float lineHeight = textScale * 10.0f;  
        
        
        float centerX = headScreen.x;
        
        
        float textY = footY + 8.0f;  
        
        
        if (showName)
        {
            int nameLen = 0;
            while (name[nameLen]) nameLen++;
            float nameWidth = nameLen * charWidth;
            float nameX = centerX - nameWidth / 2.0f;
            
            if (isPlayer)
                DrawTextD3D(nameX, textY, name, g_ColorPlayerBox[0], g_ColorPlayerBox[1], g_ColorPlayerBox[2], 1.0f, textScale);  
            else
                DrawTextD3D(nameX, textY, name, g_ColorZombieBox[0], g_ColorZombieBox[1], g_ColorZombieBox[2], 1.0f, textScale);  
            
            textY += lineHeight;  
        }
        
        
        if (showDist)
        {
            char distText[32];
            int dpos = 0;
            distText[dpos++] = '[';
            
            if (distance < 0)
            {
                
                distText[dpos++] = '?';
            }
            else
            {
                char distStr[16];
                IntToStr(distance, distStr);
                for (int i = 0; distStr[i] && dpos < 20; i++) distText[dpos++] = distStr[i];
                distText[dpos++] = 'm';
            }
            distText[dpos++] = ']';
            distText[dpos] = 0;
            
            float distWidth = dpos * charWidth;
            float distX = centerX - distWidth / 2.0f;
            
            
            if (isPlayer)
                DrawTextD3D(distX, textY, distText, g_ColorPlayerBox[0], g_ColorPlayerBox[1], g_ColorPlayerBox[2], 1.0f, textScale);
            else
                DrawTextD3D(distX, textY, distText, g_ColorZombieBox[0], g_ColorZombieBox[1], g_ColorZombieBox[2], 1.0f, textScale);
        }
    }
}

static int g_DebugCounter = 0;



static bool IsNaN(float f)
{
    return (f != f);
}


static bool GetEntityPosition(uintptr_t entity, Vec3& outPos)
{
    static int getEntityPosLogCount = 0;
    char buf[256];
    
    if (getEntityPosLogCount < 5)
    {
        wsprintfA(buf, "[GETPOS_DEBUG] GetEntityPosition: entity=0x%X%08X", 
            (DWORD)(entity >> 32), (DWORD)entity);
        Log(buf);
    }
    
    uintptr_t visualStateAddr = entity + 0x1C8;
    if (getEntityPosLogCount < 5)
    {
        wsprintfA(buf, "[GETPOS_DEBUG] Reading visualState from entity+0x1C8=0x%X%08X IsValidPtr=%d", 
            (DWORD)(visualStateAddr >> 32), (DWORD)visualStateAddr, IsValidPtr(visualStateAddr) ? 1 : 0);
        Log(buf);
    }
    
    uintptr_t visualState = Read<uintptr_t>(visualStateAddr);
    
    if (getEntityPosLogCount < 5)
    {
        wsprintfA(buf, "[GETPOS_DEBUG] Read visualState=0x%X%08X IsValidPtr=%d", 
            (DWORD)(visualState >> 32), (DWORD)visualState, IsValidPtr(visualState) ? 1 : 0);
        Log(buf);
    }
    
    if (!IsValidPtr(visualState) || visualState < 0x100000000) return false;
    
    uintptr_t posAddr = visualState + 0x2C;
    if (getEntityPosLogCount < 5)
    {
        wsprintfA(buf, "[GETPOS_DEBUG] Reading position from visualState+0x2C=0x%X%08X IsValidPtr=%d", 
            (DWORD)(posAddr >> 32), (DWORD)posAddr, IsValidPtr(posAddr) ? 1 : 0);
        Log(buf);
    }
    
    outPos.x = Read<float>(posAddr);
    outPos.y = Read<float>(posAddr + 4);
    outPos.z = Read<float>(posAddr + 8);
    
    if (getEntityPosLogCount < 5)
    {
        wsprintfA(buf, "[GETPOS_DEBUG] Read position: x=%.1f y=%.1f z=%.1f", outPos.x, outPos.y, outPos.z);
        Log(buf);
        getEntityPosLogCount++;
    }
    
    
    
    if (IsNaN(outPos.x) || IsNaN(outPos.y) || IsNaN(outPos.z)) return false;
    
    if (outPos.x == 0.0f && outPos.y == 0.0f && outPos.z == 0.0f) return false;
    if (outPos.x < -100000.0f || outPos.x > 100000.0f) return false;
    if (outPos.z < -100000.0f || outPos.z > 100000.0f) return false;
    
    return true;
}


static bool GetItemPosition(uintptr_t entity, Vec3& outPos)
{
    static int debugCallCount = 0;
    char buf[256];
    
    
    if (GetEntityPosition(entity, outPos))
    {
        
        
        bool hasNaN = (IsNaN(outPos.x) || IsNaN(outPos.y) || IsNaN(outPos.z));
        bool hasInf = (!hasNaN && (outPos.x != outPos.x || outPos.y != outPos.y || outPos.z != outPos.z));
        
        if (hasNaN || hasInf)
        {
            if (debugCallCount < 3)
            {
                wsprintfA(buf, "GetItemPosition: GetEntityPosition returned NaN/Inf values pos=(%.1f, %.1f, %.1f)", 
                    outPos.x, outPos.y, outPos.z);
                Log(buf);
                debugCallCount++;
            }
            return false; 
        }
        
        
        if (outPos.x < -1000000.0f || outPos.x > 1000000.0f || 
            outPos.z < -1000000.0f || outPos.z > 1000000.0f)
        {
            if (debugCallCount < 3)
            {
                wsprintfA(buf, "GetItemPosition: GetEntityPosition returned out-of-range values pos=(%.1f, %.1f, %.1f)", 
                    outPos.x, outPos.y, outPos.z);
                Log(buf);
                debugCallCount++;
            }
            return false;
        }
        
        if (debugCallCount < 3)
        {
            wsprintfA(buf, "GetItemPosition: GetEntityPosition succeeded pos=(%.1f, %.1f, %.1f)", 
                outPos.x, outPos.y, outPos.z);
            Log(buf);
            debugCallCount++;
        }
        return true;
    }
    
    if (debugCallCount < 3)
    {
        wsprintfA(buf, "GetItemPosition: GetEntityPosition failed, trying Transform matrix");
        Log(buf);
    }
    
    
    uintptr_t visualStateAddr = entity + 0x1C8;
    if (!IsValidPtr(visualStateAddr) || visualStateAddr < 0x100000000) 
    {
        if (debugCallCount < 3)
        {
            wsprintfA(buf, "GetItemPosition: visualState address invalid (0x%X%08X)", 
                (DWORD)(visualStateAddr >> 32), (DWORD)visualStateAddr);
            Log(buf);
        }
    }
    else
    {
        uintptr_t visualState = Read<uintptr_t>(visualStateAddr);
        if (IsValidPtr(visualState) && visualState > 0x100000000)
        {
            
            uintptr_t matrixBase = visualState + 0x8;
            if (IsValidPtr(matrixBase) && matrixBase > 0x100000000)
            {
                
                
                uintptr_t m9Addr = matrixBase + 9 * 4;
                uintptr_t m10Addr = matrixBase + 10 * 4;
                uintptr_t m11Addr = matrixBase + 11 * 4;
                
                if (IsValidPtr(m9Addr) && IsValidPtr(m10Addr) && IsValidPtr(m11Addr))
                {
                    float m9 = Read<float>(m9Addr);   
                    float m10 = Read<float>(m10Addr); 
                    float m11 = Read<float>(m11Addr); 
        
                    if (debugCallCount < 3)
                    {
                        wsprintfA(buf, "GetItemPosition: Matrix values m9=%.1f m10=%.1f m11=%.1f", m9, m10, m11);
                        Log(buf);
                    }
                    
                    
                    if (!IsNaN(m9) && !IsNaN(m10) && !IsNaN(m11))
                    {
                        if (m9 != 0.0f || m10 != 0.0f || m11 != 0.0f)
                        {
                            if (m9 >= -100000.0f && m9 <= 100000.0f && m11 >= -100000.0f && m11 <= 100000.0f)
                            {
                                outPos.x = m9;
                                outPos.y = m10;
                                outPos.z = m11;
                                if (debugCallCount < 3)
                                {
                                    wsprintfA(buf, "GetItemPosition: Matrix method succeeded");
                                    Log(buf);
                                    debugCallCount++;
                                }
                                return true;
                            }
                        }
                    }
                }
            }
        }
        else if (debugCallCount < 3)
        {
            wsprintfA(buf, "GetItemPosition: visualState invalid or null (0x%X%08X)", 
                (DWORD)(visualState >> 32), (DWORD)visualState);
            Log(buf);
        }
    }
    
    
    if (debugCallCount < 3)
    {
        Log("getpos trying fallback");
    }
    
    uintptr_t offsets[] = {
        0x2C,   
        0x8,    
        0x1C,   
        0x30,   
        0x1C8,  
    };
    
    for (int i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++)
    {
        float x = Read<float>(entity + offsets[i]);
        float y = Read<float>(entity + offsets[i] + 4);
        float z = Read<float>(entity + offsets[i] + 8);
        
        
        if (IsNaN(x) || IsNaN(y) || IsNaN(z)) continue;
        
        if (x != 0.0f || y != 0.0f || z != 0.0f)
        {
            if (x >= -100000.0f && x <= 100000.0f && z >= -100000.0f && z <= 100000.0f)
            {
                outPos.x = x;
                outPos.y = y;
                outPos.z = z;
                if (debugCallCount < 3)
                {
                    wsprintfA(buf, "GetItemPosition: Fallback offset 0x%X succeeded pos=(%.1f, %.1f, %.1f)", 
                        offsets[i], outPos.x, outPos.y, outPos.z);
                    Log(buf);
                    debugCallCount++;
                }
                return true;
            }
        }
    }
    
    if (debugCallCount < 3)
    {
        Log("getpos all failed");
        debugCallCount++;
    }
    
    return false;
}


static void DrawEntitySimple(uintptr_t entity, bool isPlayer, const char* name, int distance)
{
    Vec3 pos;
    if (!GetEntityPosition(entity, pos)) return;
    
    
    Vec3 head = pos;
    head.y += isPlayer ? 1.8f : 1.5f;
    
    Vec3 feet = pos;
    
    Vec2 headScreen, feetScreen;
    if (!WorldToScreen(head, headScreen)) return;
    if (!WorldToScreen(feet, feetScreen)) return;
    
    float height = feetScreen.y - headScreen.y;
    if (height < 2.0f) return;  
    
    float width = height / 2.5f;
    float x = headScreen.x - width / 2.0f;
    float y = headScreen.y;
    
    
    if (isPlayer && g_PlayerBox)
        AddBox(x, y, width, height, g_ColorPlayerBox[0], g_ColorPlayerBox[1], g_ColorPlayerBox[2], g_ColorPlayerBox[3]);
    else if (!isPlayer && g_ZombieBox)
        AddBox(x, y, width, height, g_ColorZombieBox[0], g_ColorZombieBox[1], g_ColorZombieBox[2], g_ColorZombieBox[3]);
    
    
    bool showName = (isPlayer && g_PlayerName) || (!isPlayer && g_ZombieName);
    bool showDist = (isPlayer && g_PlayerDistanceEnabled) || (!isPlayer && g_ZombieDistanceEnabled);
    
    if (showName || showDist)
    {
        
        float textScale = 1.8f;
        float charWidth = textScale * 6.0f;
        float lineHeight = textScale * 10.0f;
        
        
        float centerX = headScreen.x;
        float textY = feetScreen.y + 8.0f;
        
        
        if (showName)
        {
            int nameLen = 0;
            while (name[nameLen]) nameLen++;
            float nameWidth = nameLen * charWidth;
            float nameX = centerX - nameWidth / 2.0f;
            
            if (isPlayer)
                DrawTextD3D(nameX, textY, name, g_ColorPlayerBox[0], g_ColorPlayerBox[1], g_ColorPlayerBox[2], 1.0f, textScale);
            else
                DrawTextD3D(nameX, textY, name, g_ColorZombieBox[0], g_ColorZombieBox[1], g_ColorZombieBox[2], 1.0f, textScale);
            
            textY += lineHeight;
        }
        
        
        if (showDist)
        {
            char distText[32];
            int dpos = 0;
            distText[dpos++] = '[';
            
            if (distance < 0)
            {
                
                distText[dpos++] = '?';
            }
            else
            {
                char distStr[16];
                IntToStr(distance, distStr);
                for (int i = 0; distStr[i] && dpos < 20; i++) distText[dpos++] = distStr[i];
                distText[dpos++] = 'm';
            }
            distText[dpos++] = ']';
            distText[dpos] = 0;
            
            float distWidth = dpos * charWidth;
            float distX = centerX - distWidth / 2.0f;
            
            if (isPlayer)
                DrawTextD3D(distX, textY, distText, g_ColorPlayerBox[0], g_ColorPlayerBox[1], g_ColorPlayerBox[2], 1.0f, textScale);
            else
                DrawTextD3D(distX, textY, distText, g_ColorZombieBox[0], g_ColorZombieBox[1], g_ColorZombieBox[2], 1.0f, textScale);
        }
    }
}


static void DrawAnimal(uintptr_t entity, const char* name, int distance)
{
    Vec3 pos;
    if (!GetEntityPosition(entity, pos)) return;
    
    
    Vec3 top = pos;
    top.y += 1.2f;
    Vec3 bottom = pos;
    
    
    Vec2 topScreen, bottomScreen;
    if (!WorldToScreen(top, topScreen)) return;
    if (!WorldToScreen(bottom, bottomScreen)) return;
    
    
    float height = bottomScreen.y - topScreen.y;
    if (height < 2.0f) return;  
    
    float width = height / 2.0f;  
    float x = topScreen.x - width / 2.0f;
    float y = topScreen.y;
    
    
    if (g_AnimalBox)
        AddBox(x, y, width, height, g_ColorAnimalBox[0], g_ColorAnimalBox[1], g_ColorAnimalBox[2], g_ColorAnimalBox[3]);
    
    
    if (g_AnimalName || g_AnimalDistanceEnabled)
    {
        float textScale = 1.8f;
        float charWidth = textScale * 6.0f;
        float lineHeight = textScale * 10.0f;
        float centerX = topScreen.x;
        float textY = bottomScreen.y + 8.0f;
        
        
        if (g_AnimalName && name && name[0])
        {
            int nameLen = 0;
            while (name[nameLen]) nameLen++;
            float nameWidth = nameLen * charWidth;
            float nameX = centerX - nameWidth / 2.0f;
            DrawTextD3D(nameX, textY, name, g_ColorAnimalBox[0], g_ColorAnimalBox[1], g_ColorAnimalBox[2], 1.0f, textScale);
            textY += lineHeight;
        }
        
        
        if (g_AnimalDistanceEnabled && distance > 0)
        {
            char distText[32];
            int dpos = 0;
            distText[dpos++] = '[';
            char distStr[16];
            IntToStr(distance, distStr);
            for (int i = 0; distStr[i] && dpos < 20; i++) distText[dpos++] = distStr[i];
            distText[dpos++] = 'm';
            distText[dpos++] = ']';
            distText[dpos] = 0;
            
            float distWidth = dpos * charWidth;
            float distX = centerX - distWidth / 2.0f;
            
            float dr = 0.7f, dg = 0.7f, db = 0.7f;
            if (distance < 50) { dr = 0.2f; dg = 1.0f; db = 0.2f; }
            else if (distance < 150) { dr = 1.0f; dg = 1.0f; db = 0.2f; }
            else if (distance < 300) { dr = 1.0f; dg = 0.5f; db = 0.2f; }
            else { dr = 1.0f; dg = 0.3f; db = 0.3f; }
            
            DrawTextD3D(distX, textY, distText, dr, dg, db, 1.0f, textScale);
        }
    }
}


static void DrawVehicle(uintptr_t entity, const char* name, int distance)
{
    Vec3 pos;
    if (!GetEntityPosition(entity, pos)) return;
    
    
    Vec3 top = pos;
    top.y += 2.0f;
    Vec3 bottom = pos;
    
    
    Vec2 topScreen, bottomScreen;
    if (!WorldToScreen(top, topScreen)) return;
    if (!WorldToScreen(bottom, bottomScreen)) return;
    
    
    float height = bottomScreen.y - topScreen.y;
    if (height < 2.0f) return;  
    
    float width = height * 1.5f;  
    float x = topScreen.x - width / 2.0f;
    float y = topScreen.y;
    
    
    if (g_VehicleBox)
        AddBox(x, y, width, height, g_ColorVehicleBox[0], g_ColorVehicleBox[1], g_ColorVehicleBox[2], g_ColorVehicleBox[3]);
    
    
    if (g_VehicleName || g_VehicleDistanceEnabled)
    {
        float textScale = 1.8f;
        float charWidth = textScale * 6.0f;
        float lineHeight = textScale * 10.0f;
        float centerX = topScreen.x;
        float textY = bottomScreen.y + 8.0f;
        
        
        if (g_VehicleName && name && name[0])
        {
            int nameLen = 0;
            while (name[nameLen]) nameLen++;
            float nameWidth = nameLen * charWidth;
            float nameX = centerX - nameWidth / 2.0f;
            DrawTextD3D(nameX, textY, name, g_ColorVehicleBox[0], g_ColorVehicleBox[1], g_ColorVehicleBox[2], 1.0f, textScale);
            textY += lineHeight;
        }
        
        
        if (g_VehicleDistanceEnabled && distance > 0)
        {
            char distText[32];
            int dpos = 0;
            distText[dpos++] = '[';
            char distStr[16];
            IntToStr(distance, distStr);
            for (int i = 0; distStr[i] && dpos < 20; i++) distText[dpos++] = distStr[i];
            distText[dpos++] = 'm';
            distText[dpos++] = ']';
            distText[dpos] = 0;
            
            float distWidth = dpos * charWidth;
            float distX = centerX - distWidth / 2.0f;
            
            float dr = 0.7f, dg = 0.7f, db = 0.7f;
            if (distance < 50) { dr = 0.2f; dg = 1.0f; db = 0.2f; }
            else if (distance < 150) { dr = 1.0f; dg = 1.0f; db = 0.2f; }
            else if (distance < 300) { dr = 1.0f; dg = 0.5f; db = 0.2f; }
            else { dr = 1.0f; dg = 0.3f; db = 0.3f; }
            
            DrawTextD3D(distX, textY, distText, dr, dg, db, 1.0f, textScale);
        }
    }
}


static void DrawItem(uintptr_t entity, const char* name, int distance)
{
    static int debugDrawCount = 0;
    char buf[256];
    
    Vec3 pos;
    
    if (!GetItemPosition(entity, pos))
    {
        if (debugDrawCount < 3)
        {
            Log("draw getpos failed");
            debugDrawCount++;
        }
        return;
    }
    
    
    
    if (IsNaN(pos.x) || IsNaN(pos.y) || IsNaN(pos.z))
    {
        if (debugDrawCount < 3)
        {
            wsprintfA(buf, "draw: Position contains NaN values (%.1f, %.1f, %.1f), skipping", pos.x, pos.y, pos.z);
            Log(buf);
            debugDrawCount++;
        }
        return;
    }
    
    if (debugDrawCount < 3)
    {
        wsprintfA(buf, "draw: pos=(%.1f, %.1f, %.1f) name='%s'", pos.x, pos.y, pos.z, name);
        Log(buf);
    }
    
    Vec2 screen;
    if (!WorldToScreen(pos, screen))
    {
        if (debugDrawCount < 3)
        {
            Log("draw w2s failed");
            debugDrawCount++;
        }
        return;
    }
    
    if (debugDrawCount < 3)
    {
        wsprintfA(buf, "draw: screen=(%.1f, %.1f) drawing item", screen.x, screen.y);
        Log(buf);
        debugDrawCount++;
    }
    
    
    float boxSize = 8.0f;
    float x = screen.x - boxSize / 2;
    float y = screen.y - boxSize / 2;
    
    
    if (g_ItemBox)
        AddBox(x, y, boxSize, boxSize, g_ColorItemBox[0], g_ColorItemBox[1], g_ColorItemBox[2], g_ColorItemBox[3]);
    
    
    if (g_ItemName || g_ItemDistanceEnabled)
    {
        char infoText[128];
        int pos2 = 0;
        
        if (g_ItemName && name && name[0])
        {
            for (int i = 0; name[i] && pos2 < 90; i++)
                infoText[pos2++] = name[i];
        }
        
        if (g_ItemDistanceEnabled && distance > 0)
        {
            char distStr[16];
            IntToStr(distance, distStr);
            
            if (pos2 > 0) infoText[pos2++] = ' ';
            infoText[pos2++] = '[';
            for (int i = 0; distStr[i] && pos2 < 110; i++) infoText[pos2++] = distStr[i];
            infoText[pos2++] = 'm';
            infoText[pos2++] = ']';
        }
        infoText[pos2] = 0;
        
        if (pos2 > 0)
        {
            DrawTextD3D(x, y + boxSize + 2, infoText, g_ColorItemBox[0], g_ColorItemBox[1], g_ColorItemBox[2], 1.0f, 1.5f);
        }
    }
}


static void RenderESP()
{
    if (!g_GameModule || !g_ShowESP) return;
    
    char buf[256];
    
    
    uintptr_t worldPtr = 0;
    
    uintptr_t worldOffsets[] = { 0xF4B0A0, 0xF4B050, 0xF4A0D0 };
    
    for (int w = 0; w < 3; w++)
    {
        uintptr_t test = Read<uintptr_t>((uintptr_t)g_GameModule + worldOffsets[w]);
        if (g_DebugCounter == 0)
        {
            wsprintfA(buf, "Trying World offset 0x%X: ptr=0x%X%08X", 
                worldOffsets[w], (DWORD)(test >> 32), (DWORD)test);
            Log(buf);
        }
        
        if (IsValidPtr(test) && test > 0x100000000)
        {
            uintptr_t camTest = Read<uintptr_t>(test + 0x1B8);
            if (g_DebugCounter == 0)
            {
                wsprintfA(buf, "  Camera at 0x1B8: ptr=0x%X%08X", 
                    (DWORD)(camTest >> 32), (DWORD)camTest);
                Log(buf);
            }
            
            
            if (IsValidPtr(camTest) && camTest > 0x100000000)
            {
                worldPtr = test;
                if (g_DebugCounter == 0)
                {
                    wsprintfA(buf, "World found at offset 0x%X = 0x%X%08X (camera valid)", 
                        worldOffsets[w], (DWORD)(worldPtr >> 32), (DWORD)worldPtr);
                    Log(buf);
                }
                break;
            }
            else
            {
                
                uintptr_t testNearList = Read<uintptr_t>(test + 0xF48);
                uintptr_t testFarList = Read<uintptr_t>(test + 0x1090);
                if (g_DebugCounter == 0)
                {
                    wsprintfA(buf, "  Testing lists: NearList=0x%X%08X FarList=0x%X%08X", 
                        (DWORD)(testNearList >> 32), (DWORD)testNearList,
                        (DWORD)(testFarList >> 32), (DWORD)testFarList);
                    Log(buf);
                }
                
                
                if ((IsValidPtr(testNearList) && testNearList > 0x100000000) ||
                    (IsValidPtr(testFarList) && testFarList > 0x100000000))
                {
                    worldPtr = test;
                    if (g_DebugCounter == 0)
                    {
                        wsprintfA(buf, "World found at offset 0x%X = 0x%X%08X (lists valid, camera invalid)", 
                            worldOffsets[w], (DWORD)(worldPtr >> 32), (DWORD)worldPtr);
                        Log(buf);
                    }
                    break;
                }
            }
        }
    }
    
    if (worldPtr == 0)
    {
        if (g_DebugCounter == 0) { Log("no world ptr"); g_DebugCounter = 1; }
        return;
    }
    
    
    uintptr_t camera = Read<uintptr_t>(worldPtr + 0x1B8);
    
    
    uintptr_t localPlayer = Read<uintptr_t>(worldPtr + 0x2960);
    
    if (g_DebugCounter == 0)
    {
        wsprintfA(buf, "Camera=0x%X%08X LocalPlayer=0x%X%08X", 
            (DWORD)(camera >> 32), (DWORD)camera,
            (DWORD)(localPlayer >> 32), (DWORD)localPlayer);
        Log(buf);
        
        
        if (IsValidPtr(camera) && camera > 0x100000000)
        {
            Vec3 camPos = Read<Vec3>(camera + 0x2C);  
            Vec3 projD1 = Read<Vec3>(camera + 0xD0);  
            
            wsprintfA(buf, "CamPos: X=%d Y=%d Z=%d ProjD1.x=%d", 
                (int)camPos.x, (int)camPos.y, (int)camPos.z, (int)(projD1.x * 1000));
            Log(buf);
        }
        
        
        if (IsValidPtr(localPlayer) && localPlayer > 0x100000000)
        {
            Vec3 localPos;
            if (GetEntityPosition(localPlayer, localPos))
            {
                wsprintfA(buf, "LocalPlayer Pos: X=%d Y=%d Z=%d", 
                    (int)localPos.x, (int)localPos.y, (int)localPos.z);
                Log(buf);
            }
            else
            {
                Log("localplayer pos failed");
            }
        }
    }
    
    
    
    g_CameraValid = false;
    if (IsValidPtr(camera) && camera > 0x100000000)
    {
        g_CameraPos = Read<Vec3>(camera + 0x2C);
        if (g_CameraPos.x != 0.0f || g_CameraPos.y != 0.0f || g_CameraPos.z != 0.0f)
        {
            g_CameraValid = true;
        }
    }
    
    g_LocalPlayerValid = false;
    if (IsValidPtr(localPlayer) && localPlayer > 0x100000000)
    {
        
        
        uintptr_t visualState = Read<uintptr_t>(localPlayer + 0x1C8);
        if (IsValidPtr(visualState) && visualState > 0x100000000)
        {
            
            
            float m9 = Read<float>(visualState + 0x8 + 9 * 4);   
            float m10 = Read<float>(visualState + 0x8 + 10 * 4); 
            float m11 = Read<float>(visualState + 0x8 + 11 * 4); 
            
            if (m9 != 0.0f || m10 != 0.0f || m11 != 0.0f)
            {
                g_LocalPlayerPos.x = m9;
                g_LocalPlayerPos.y = m10;
                g_LocalPlayerPos.z = m11;
                g_LocalPlayerValid = true;
            }
        }
        
        
        if (!g_LocalPlayerValid)
        {
            Vec3 pelvisPos = GetBonePosition(localPlayer, BONE_PELVIS, true);
            if (pelvisPos.x != 0.0f || pelvisPos.y != 0.0f || pelvisPos.z != 0.0f)
            {
                g_LocalPlayerPos = pelvisPos;
                g_LocalPlayerValid = true;
            }
        }
        
        
        if (!g_LocalPlayerValid)
        {
            if (GetEntityPosition(localPlayer, g_LocalPlayerPos))
            {
                g_LocalPlayerValid = true;
            }
        }
    }
    
    
    
    if (g_SharedConfig && g_SharedConfig->magic == 0x4F414B00)
    {
        if (g_LocalPlayerValid)
        {
            g_SharedConfig->PlayerX = g_LocalPlayerPos.z;  
            g_SharedConfig->PlayerY = g_LocalPlayerPos.x;  
            g_SharedConfig->PlayerZ = g_LocalPlayerPos.y;  
        }
        else if (g_CameraValid)
        {
            g_SharedConfig->PlayerX = g_CameraPos.z;  
            g_SharedConfig->PlayerY = g_CameraPos.x;  
            g_SharedConfig->PlayerZ = g_CameraPos.y;  
        }
    }
    
    
    struct EntityListInfo {
        uintptr_t offset;
        uintptr_t countOffset;
        const char* name;
        bool isSlowList;  
    };
    
    
    
    
    
    
    
    EntityListInfo lists[] = {
        { 0xF48,  0xF48 + 0x8,  "NearEntList", false },   
        { 0x1090, 0x1090 + 0x8, "FarEntList", false },    
        { 0x2010, 0x1F90, "SlowEntList", true },          
        { 0x2060, 0x2060 + 0x8, "ItemTable", true },     
    };
    
    int totalDrawn = 0;
    
    
    bool itemTableValid = true;
    
    for (int listIdx = 0; listIdx < 4; listIdx++)
    {
        uintptr_t entListPtr = 0;
        int entCount = 0;
        
        uintptr_t listAddr = worldPtr + lists[listIdx].offset;
        
        
        
        if (lists[listIdx].isSlowList)
        {
            
            
            
            entListPtr = Read<uintptr_t>(listAddr);
            
            if (listIdx == 2) 
            {
                entCount = Read<int>(worldPtr + 0x1F90);
                
                if (entCount > 2000 || entCount < 0)
                {
                    DWORD countDword = Read<DWORD>(worldPtr + 0x1F90);
                    if (countDword < 2000) entCount = (int)countDword;
                }
            }
            else if (listIdx == 3) 
            {
                
                entCount = Read<int>(worldPtr + 0x2070);
                
                if (entCount > 200000 || entCount < 0)
                {
                    DWORD countDword = Read<DWORD>(worldPtr + 0x2070);
                    if (countDword < 200000) entCount = (int)countDword;
                }
            }
            
            
            
            if (!IsValidPtr(entListPtr) || entListPtr == 0 || entListPtr < 0x100000000)
            {
                entListPtr = 0;
                entCount = 0;
                
                if (listIdx == 3)
                {
                    itemTableValid = false;
                    if (g_DebugCounter == 0)
                    {
                        wsprintfA(buf, "item: ItemTable pointer invalid (0x%X%08X) - marking itemTableValid=false", 
                            (DWORD)(entListPtr >> 32), (DWORD)entListPtr);
                        Log(buf);
                    }
                }
            }
            else
            {
                
                if (listIdx == 3)
                {
                    itemTableValid = true;
                    if (g_DebugCounter == 0)
                    {
                        wsprintfA(buf, "item: ItemTable pointer valid (0x%X%08X) count=%d - marking itemTableValid=true", 
                            (DWORD)(entListPtr >> 32), (DWORD)entListPtr, entCount);
                        Log(buf);
                    }
                }
            }
        }
        else
        {
            
            
            
            uintptr_t qword = Read<uintptr_t>(listAddr);
            
            if (g_DebugCounter == 0)
            {
                wsprintfA(buf, "%s: Reading from worldPtr+offset: worldPtr=0x%X%08X offset=0x%X addr=0x%X%08X qword=0x%X%08X",
                    lists[listIdx].name,
                    (DWORD)(worldPtr >> 32), (DWORD)worldPtr,
                    lists[listIdx].offset,
                    (DWORD)(listAddr >> 32), (DWORD)listAddr,
                    (DWORD)(qword >> 32), (DWORD)qword);
                Log(buf);
            }
            
            if (IsValidPtr(qword) && qword > 0x100000000)
            {
                
                entListPtr = Read<uintptr_t>(qword);
                entCount = Read<int>(qword + 8);
                
                
                int maxCount = (listIdx == 3) ? 200000 : 2000;  
                if (entCount < 0 || entCount > maxCount) 
                {
                    entListPtr = 0;
                    entCount = 0;
                }
                
                if (g_DebugCounter == 0 && IsValidPtr(entListPtr) && entListPtr > 0x100000000 && entCount > 0)
                {
                    wsprintfA(buf, "%s Layout A: qword=0x%X%08X dataPtr=0x%X%08X count=%d",
                        lists[listIdx].name,
                        (DWORD)(qword >> 32), (DWORD)qword,
                        (DWORD)(entListPtr >> 32), (DWORD)entListPtr, entCount);
                    Log(buf);
                }
                else if (g_DebugCounter == 0)
                {
                    wsprintfA(buf, "%s Layout A: qword=0x%X%08X but dataPtr invalid or count invalid (dataPtr=0x%X%08X count=%d)",
                        lists[listIdx].name,
                        (DWORD)(qword >> 32), (DWORD)qword,
                        (DWORD)(entListPtr >> 32), (DWORD)entListPtr, entCount);
                    Log(buf);
                }
            }
            
            
            if (!IsValidPtr(entListPtr) || entListPtr == 0 || entCount <= 0)
            {
                
                if (IsValidPtr(listAddr) && listAddr > 0x100000000)
                {
                    entListPtr = Read<uintptr_t>(listAddr);
                    entCount = Read<int>(listAddr + 8);
                    
                    
                    int maxCount = (listIdx == 3) ? 200000 : 2000;  
                    if (entCount < 0 || entCount > maxCount) 
                    {
                        entListPtr = 0;
                        entCount = 0;
                    }
                    
                    if (g_DebugCounter == 0)
                    {
                        wsprintfA(buf, "%s Layout B: dataPtr=0x%X%08X count=%d (read from addr=0x%X%08X)",
                            lists[listIdx].name,
                            (DWORD)(entListPtr >> 32), (DWORD)entListPtr, entCount,
                            (DWORD)(listAddr >> 32), (DWORD)listAddr);
                        Log(buf);
                    }
                }
                else if (g_DebugCounter == 0)
                {
                    wsprintfA(buf, "%s Layout B: listAddr invalid (0x%X%08X)", lists[listIdx].name,
                        (DWORD)(listAddr >> 32), (DWORD)listAddr);
                    Log(buf);
                }
            }
        }
        
        if (g_DebugCounter == 0)
        {
            wsprintfA(buf, "%s: offset=0x%X ptr=0x%X%08X count=%d (raw int=%d)",
                lists[listIdx].name, lists[listIdx].offset, 
                (DWORD)(entListPtr >> 32), (DWORD)entListPtr, entCount,
                Read<int>(worldPtr + (lists[listIdx].isSlowList ? 0x1F90 : lists[listIdx].countOffset)));
            Log(buf);
            
            
            uintptr_t rawValue = Read<uintptr_t>(worldPtr + lists[listIdx].offset);
            wsprintfA(buf, "%s: raw memory at 0x%X = 0x%X%08X (worldPtr=0x%X%08X, addr=0x%X%08X)",
                lists[listIdx].name, lists[listIdx].offset,
                (DWORD)(rawValue >> 32), (DWORD)rawValue,
                (DWORD)(worldPtr >> 32), (DWORD)worldPtr,
                (DWORD)((worldPtr + lists[listIdx].offset) >> 32), (DWORD)(worldPtr + lists[listIdx].offset));
            Log(buf);
            
            
            if (!lists[listIdx].isSlowList && listIdx != 3)
            {
                
                uintptr_t oldPtr = Read<uintptr_t>(worldPtr + lists[listIdx].offset);
                int oldCount = Read<int>(worldPtr + lists[listIdx].countOffset);
                wsprintfA(buf, "%s: OLD METHOD - ptr=0x%X%08X count=%d",
                    lists[listIdx].name,
                    (DWORD)(oldPtr >> 32), (DWORD)oldPtr, oldCount);
                Log(buf);
            }
        }
        
        
        if (!IsValidPtr(entListPtr) || entListPtr == 0 || entListPtr < 0x100000000)
        {
            if (g_DebugCounter == 0)
            {
                wsprintfA(buf, "%s: Skipping - invalid pointer (0x%X%08X)", lists[listIdx].name,
                    (DWORD)(entListPtr >> 32), (DWORD)entListPtr);
                Log(buf);
            }
            
            if (listIdx == 3)
            {
                itemTableValid = false;
                if (g_DebugCounter == 0)
                {
                    Log("item: ItemTable is invalid - will skip ALL items from listIdx=3");
                }
            }
            continue; 
        }
        
        
        
        
        
        int maxCount = (listIdx == 3) ? 200000 : 2000;  
        if (entCount <= 0 || entCount > maxCount)
        {
            if (g_DebugCounter == 0)
            {
                wsprintfA(buf, "%s: Skipping - invalid count %d (max=%d)", lists[listIdx].name, entCount, maxCount);
                Log(buf);
            }
            
            if (listIdx == 3)
            {
                itemTableValid = false;
            }
            continue;
        }
        
        
        
        if (listIdx == 3)
        {
            
            if (itemTableValid && IsValidPtr(entListPtr) && entListPtr != 0 && entListPtr >= 0x100000000)
            {
                
                itemTableValid = true;
                if (g_DebugCounter == 0)
                {
                    wsprintfA(buf, "item: ItemTable VALIDATED: ptr=0x%X%08X count=%d - setting itemTableValid=true", 
                        (DWORD)(entListPtr >> 32), (DWORD)entListPtr, entCount);
                    Log(buf);
                }
            }
            else
            {
                
                itemTableValid = false;
                if (g_DebugCounter == 0)
                {
                    wsprintfA(buf, "item: ItemTable NOT VALIDATED: ptr=0x%X%08X is invalid - keeping itemTableValid=false", 
                        (DWORD)(entListPtr >> 32), (DWORD)entListPtr);
                    Log(buf);
                }
            }
        }
        
        
        
        
        if (listIdx == 3)
        {
            
            if (!IsValidPtr(entListPtr) || entListPtr == 0 || entListPtr < 0x100000000)
            {
                itemTableValid = false;
                if (g_DebugCounter == 0)
                {
                    wsprintfA(buf, "item: ItemTable invalid - ptr=0x%X%08X - SKIPPING ENTIRE LIST", 
                        (DWORD)(entListPtr >> 32), (DWORD)entListPtr);
                    Log(buf);
                }
                continue; 
            }
            
            
            if (!itemTableValid)
            {
                if (g_DebugCounter == 0)
                {
                    Log("item table invalid skip list");
                }
                continue; 
            }
        }
        
        
        
        int maxIter;
        if (listIdx == 3 && !itemTableValid)
        {
            maxIter = 0; 
        }
        else
        {
            maxIter = (listIdx == 3) ? ((entCount > 500) ? 500 : entCount) : ((entCount > 100) ? 100 : entCount);
        }
        
        
        if (listIdx == 3 && !itemTableValid)
        {
            if (g_DebugCounter == 0)
            {
                Log("item: FINAL CHECK: ItemTable invalid - maxIter=0, NOT entering entity loop");
            }
            continue; 
        }
        
        
        __try {
        for (int i = 0; i < maxIter; i++)
        {
            
            if (listIdx == 3 && !itemTableValid)
            {
                break; 
            }
            
            
            if (listIdx == 3 && (entListPtr == 0 || !IsValidPtr(entListPtr) || entListPtr < 0x100000000))
            {
                break; 
            }
            
            if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
            {
                wsprintfA(buf, "item: listIdx=%d i=%d: Starting entity read entListPtr=0x%X%08X itemTableValid=%d", 
                    listIdx, i, (DWORD)(entListPtr >> 32), (DWORD)entListPtr, itemTableValid ? 1 : 0);
                Log(buf);
            }
            
            uintptr_t entity = 0;
            
            if (lists[listIdx].isSlowList)
            {
                
                uintptr_t entryAddr = entListPtr + (i * 0x18);
                if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
                {
                    wsprintfA(buf, "item: ItemTable (0x18 struct): entryAddr=0x%X%08X IsValidPtr=%d", 
                        (DWORD)(entryAddr >> 32), (DWORD)entryAddr, IsValidPtr(entryAddr) ? 1 : 0);
                    Log(buf);
                }
                if (!IsValidPtr(entryAddr) || entryAddr < 0x100000000) continue;
                
                if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
                {
                    wsprintfA(buf, "item: ItemTable: Reading flag from 0x%X%08X", 
                        (DWORD)(entryAddr >> 32), (DWORD)entryAddr);
                    Log(buf);
                }
                WORD flag = Read<WORD>(entryAddr);
                if (flag != 1) 
                {
                    if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
                    {
                        wsprintfA(buf, "item: ItemTable: flag=%d (not 1), skipping", flag);
                        Log(buf);
                    }
                    continue;
                }
                
                if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
                {
                    wsprintfA(buf, "item: ItemTable: Reading entity from entryAddr+0x8=0x%X%08X", 
                        (DWORD)((entryAddr + 0x8) >> 32), (DWORD)(entryAddr + 0x8));
                    Log(buf);
                }
                entity = Read<uintptr_t>(entryAddr + 0x8);
            }
            else
            {
                
                uintptr_t entityAddr = entListPtr + (i * 0x8);
                
                if (g_DebugCounter == 0 && listIdx == 0 && i < 5)
                {
                    wsprintfA(buf, "RegularList: entityAddr=0x%X%08X IsValidPtr=%d", 
                        (DWORD)(entityAddr >> 32), (DWORD)entityAddr, IsValidPtr(entityAddr) ? 1 : 0);
                    Log(buf);
                }
                
                
                if (!IsValidPtr(entityAddr) || entityAddr < 0x100000000) continue;
                
                
                entity = Read<uintptr_t>(entityAddr);
            }
            
            if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
            {
                wsprintfA(buf, "item: Got entity=0x%X%08X IsValidPtr=%d", 
                    (DWORD)(entity >> 32), (DWORD)entity, IsValidPtr(entity) ? 1 : 0);
                Log(buf);
            }
            
            if (!IsValidPtr(entity) || entity < 0x100000000) continue;
            if (entity == localPlayer) continue;  
            
            
            uintptr_t entityTypeAddr = entity + 0x180;
            
            if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
            {
                wsprintfA(buf, "item: Reading entityType from entity+0x180=0x%X%08X IsValidPtr=%d", 
                    (DWORD)(entityTypeAddr >> 32), (DWORD)entityTypeAddr, IsValidPtr(entityTypeAddr) ? 1 : 0);
                Log(buf);
            }
            
            if (!IsValidPtr(entityTypeAddr) || entityTypeAddr < 0x100000000) continue;
            
            
            uintptr_t entityType = Read<uintptr_t>(entityTypeAddr);
            
            if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
            {
                wsprintfA(buf, "item: Read entityType=0x%X%08X IsValidPtr=%d", 
                    (DWORD)(entityType >> 32), (DWORD)entityType, IsValidPtr(entityType) ? 1 : 0);
                Log(buf);
            }
            
            if (g_DebugCounter == 0 && listIdx == 0 && i < 5)
            {
                wsprintfA(buf, "  Ent[%d]: 0x%X%08X -> EntityType: 0x%X%08X", 
                    i, (DWORD)(entity >> 32), (DWORD)entity,
                    (DWORD)(entityType >> 32), (DWORD)entityType);
                Log(buf);
            }
            
            if (!IsValidPtr(entityType) || entityType < 0x100000000) continue;
            
            
            uintptr_t configNamePtrAddr = entityType + 0xA8;
            
            if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
            {
                wsprintfA(buf, "item: Reading configNamePtr from entityType+0xA8=0x%X%08X IsValidPtr=%d", 
                    (DWORD)(configNamePtrAddr >> 32), (DWORD)configNamePtrAddr, IsValidPtr(configNamePtrAddr) ? 1 : 0);
                Log(buf);
            }
            
            if (!IsValidPtr(configNamePtrAddr) || configNamePtrAddr < 0x100000000) continue;
            
            uintptr_t configNamePtr = Read<uintptr_t>(configNamePtrAddr);
            
            if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
            {
                wsprintfA(buf, "item: Read configNamePtr=0x%X%08X IsValidPtr=%d", 
                    (DWORD)(configNamePtr >> 32), (DWORD)configNamePtr, IsValidPtr(configNamePtr) ? 1 : 0);
                Log(buf);
            }
            
            if (g_DebugCounter == 0 && listIdx == 0 && i < 5)
            {
                wsprintfA(buf, "    ConfigName ptr: 0x%X%08X", 
                    (DWORD)(configNamePtr >> 32), (DWORD)configNamePtr);
                Log(buf);
            }
            
            if (!IsValidPtr(configNamePtr) || configNamePtr < 0x100000000) continue;
            
            
            uintptr_t nameLenAddr = configNamePtr + 0x8;
            
            if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
            {
                wsprintfA(buf, "item: Reading nameLen from configNamePtr+0x8=0x%X%08X IsValidPtr=%d", 
                    (DWORD)(nameLenAddr >> 32), (DWORD)nameLenAddr, IsValidPtr(nameLenAddr) ? 1 : 0);
                Log(buf);
            }
            
            if (!IsValidPtr(nameLenAddr) || nameLenAddr < 0x100000000) continue;
            
            WORD nameLen = Read<WORD>(nameLenAddr);
            
            if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
            {
                wsprintfA(buf, "item: Read nameLen=%d", nameLen);
                Log(buf);
            }
            
            if (nameLen == 0 || nameLen > 255) continue;
            
            
            uintptr_t nameStringAddr = configNamePtr + 0x10;
            
            if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
            {
                wsprintfA(buf, "item: Reading string from configNamePtr+0x10=0x%X%08X IsValidPtr=%d nameLen=%d", 
                    (DWORD)(nameStringAddr >> 32), (DWORD)nameStringAddr, IsValidPtr(nameStringAddr) ? 1 : 0, nameLen);
                Log(buf);
            }
            
            if (!IsValidPtr(nameStringAddr) || nameStringAddr < 0x100000000) continue;
            
            char configName[64];
            ZeroMem(configName, 64);
            for (int j = 0; j < nameLen && j < 63; j++)
            {
                uintptr_t charAddr = nameStringAddr + j;
                if (!IsValidPtr(charAddr) || charAddr < 0x100000000) break;
                configName[j] = Read<char>(charAddr);
            }
            configName[nameLen] = 0;
            
            if (g_DebugCounter == 0 && listIdx == 3 && i < 10)
            {
                wsprintfA(buf, "item: Read configName='%s'", configName);
                Log(buf);
            }
            
            if (g_DebugCounter == 0 && listIdx == 0 && i < 10)
            {
                wsprintfA(buf, "    ConfigName: '%s'", configName);
                Log(buf);
            }
            
            
            bool isPlayer = (StrCmpI(configName, "dayzplayer") == 0);
            bool isZombie = (StrCmpI(configName, "dayzinfected") == 0);
            bool isAnimal = (StrCmpI(configName, "dayzanimal") == 0);
            
            bool isVehicle = (StrCmp(configName, "car") == 0) || (StrCmpI(configName, "boat") == 0);
            bool isExcluded = IsExcludedFromItems(configName);
            
            bool isItem = !isPlayer && !isZombie && !isAnimal && !isVehicle && !isExcluded && configName[0] != 0;
            
            
            if (isItem && listIdx == 3 && !itemTableValid)
            {
                if (g_DebugCounter == 0 && i < 5)
                {
                    wsprintfA(buf, "item: Skipping item from invalid ItemTable: configName='%s' listIdx=%d i=%d", 
                        configName, listIdx, i);
                    Log(buf);
                }
                continue;
            }
            
            
            
            
            
            if (isItem)
            {
                static int itemLogCount = 0;
                if (itemLogCount < 20)
                {
                    wsprintfA(buf, "Item found: configName='%s' isExcluded=%d g_ESPItems=%d listIdx=%d i=%d itemTableValid=%d", 
                        configName, isExcluded ? 1 : 0, g_ESPItems ? 1 : 0, listIdx, i, itemTableValid ? 1 : 0);
                    Log(buf);
                    itemLogCount++;
                }
            }
            
            if (isPlayer && !g_ESPPlayers) continue;
            if (isZombie && !g_ESPZombies) continue;
            if (isAnimal && !g_ESPAnimals) continue;
            if (isVehicle && !g_ESPVehicles) continue;
            if (isItem && !g_ESPItems) 
            {
                if (g_DebugCounter == 0 && i < 5)
                {
                    Log("item skipped esp off");
                }
                continue;
            }
            
            if (isPlayer || isZombie)
            {
                
                bool isDead = Read<bool>(entity + 0xE2);  
                if (isDead) continue;
                
                
                Vec3 entityPos;
                int distance = 0;
                if (GetEntityPosition(entity, entityPos))
                {
                    
                    Vec3 referencePos;
                    bool hasReference = false;
                    
                    if (g_LocalPlayerValid)
                    {
                        referencePos = g_LocalPlayerPos;
                        hasReference = true;
                    }
                    else if (g_CameraValid)
                    {
                        referencePos = g_CameraPos;
                        hasReference = true;
                    }
                    
                    if (hasReference)
                    {
                        distance = (int)Distance3D(referencePos, entityPos);
                        
                        
                        if (g_LocalPlayerValid)
                        {
                            if (isPlayer && distance > g_PlayerDistance) continue;
                            if (isZombie && distance > g_ZombieDistance) continue;
                        }
                    }
                    else
                    {
                        
                        distance = -1;
                    }
                }
                
                
                char displayName[64];
                ZeroMem(displayName, 64);
                
                
                uintptr_t typeNamePtrAddr = entityType + 0x70;
                if (IsValidPtr(typeNamePtrAddr) && typeNamePtrAddr > 0x100000000)
                {
                    uintptr_t typeNamePtr = Read<uintptr_t>(typeNamePtrAddr);  
                    if (IsValidPtr(typeNamePtr) && typeNamePtr > 0x100000000)
                    {
                        
                        uintptr_t typeNameLenAddr = typeNamePtr + 0x8;
                        if (IsValidPtr(typeNameLenAddr) && typeNameLenAddr > 0x100000000)
                        {
                            WORD typeNameLen = Read<WORD>(typeNameLenAddr);
                            if (typeNameLen > 0 && typeNameLen < 60)
                            {
                                
                                uintptr_t typeNameStringAddr = typeNamePtr + 0x10;
                                if (IsValidPtr(typeNameStringAddr) && typeNameStringAddr > 0x100000000)
                                {
                                    for (int j = 0; j < typeNameLen && j < 59; j++)
                                    {
                                        uintptr_t charAddr = typeNameStringAddr + j;
                                        if (!IsValidPtr(charAddr) || charAddr < 0x100000000) break;
                                        displayName[j] = Read<char>(charAddr);
                                    }
                                    displayName[typeNameLen] = 0;
                                }
                            }
                        }
                    }
                }
                
                
                if (displayName[0] == 0)
                {
                    if (isPlayer)
                    {
                        displayName[0] = 'P'; displayName[1] = 'l'; displayName[2] = 'a';
                        displayName[3] = 'y'; displayName[4] = 'e'; displayName[5] = 'r';
                        displayName[6] = 0;
                    }
                    else
                    {
                        displayName[0] = 'Z'; displayName[1] = 'o'; displayName[2] = 'm';
                        displayName[3] = 'b'; displayName[4] = 'i'; displayName[5] = 'e';
                        displayName[6] = 0;
                    }
                }
                
                
                if (g_ShowSkeleton && isPlayer)
                    DrawEntitySkeleton(entity, isPlayer, displayName, distance);
                else
                    DrawEntitySimple(entity, isPlayer, displayName, distance);
                    
                totalDrawn++;
                
                if (g_DebugCounter == 0)
                {
                    wsprintfA(buf, "DRAWING %s at entity 0x%X%08X", 
                        isPlayer ? "PLAYER" : "ZOMBIE",
                        (DWORD)(entity >> 32), (DWORD)entity);
                    Log(buf);
                }
            }
            else if (isAnimal)
            {
                
                Vec3 animalPos;
                int distance = 0;
                if (GetEntityPosition(entity, animalPos))
                {
                    Vec3 referencePos;
                    bool hasReference = false;
                    
                    if (g_LocalPlayerValid)
                    {
                        referencePos = g_LocalPlayerPos;
                        hasReference = true;
                    }
                    else if (g_CameraValid)
                    {
                        referencePos = g_CameraPos;
                        hasReference = true;
                    }
                    
                    if (hasReference)
                    {
                        distance = (int)Distance3D(referencePos, animalPos);
                        
                        if (g_LocalPlayerValid && distance > g_AnimalDistance) continue;
                    }
                    else
                    {
                        distance = -1;
                    }
                }
                
                
                char animalName[64];
                ZeroMem(animalName, 64);
                
                
                uintptr_t typeNamePtrAddr = entityType + 0x70;
                if (IsValidPtr(typeNamePtrAddr) && typeNamePtrAddr > 0x100000000)
                {
                    uintptr_t typeNamePtr = Read<uintptr_t>(typeNamePtrAddr);
                    if (IsValidPtr(typeNamePtr) && typeNamePtr > 0x100000000)
                    {
                        
                        uintptr_t typeNameLenAddr = typeNamePtr + 0x8;
                        if (IsValidPtr(typeNameLenAddr) && typeNameLenAddr > 0x100000000)
                        {
                            WORD typeNameLen = Read<WORD>(typeNameLenAddr);
                            if (typeNameLen > 0 && typeNameLen < 60)
                            {
                                
                                uintptr_t typeNameStringAddr = typeNamePtr + 0x10;
                                if (IsValidPtr(typeNameStringAddr) && typeNameStringAddr > 0x100000000)
                                {
                                    for (int j = 0; j < typeNameLen && j < 59; j++)
                                    {
                                        uintptr_t charAddr = typeNameStringAddr + j;
                                        if (!IsValidPtr(charAddr) || charAddr < 0x100000000) break;
                                        animalName[j] = Read<char>(charAddr);
                                    }
                                    animalName[typeNameLen] = 0;
                                }
                            }
                        }
                    }
                }
                
                
                if (animalName[0] == 0)
                {
                    for (int j = 0; configName[j] && j < 60; j++)
                        animalName[j] = configName[j];
                }
                
                
                DrawAnimal(entity, animalName, distance);
                totalDrawn++;
            }
            else if (isItem)
            {
                
                if (listIdx == 3 && !itemTableValid)
                {
                    continue; 
                }
                
                
                if (!g_ESPItems)
                {
                    continue;
                }
                
                
                if (!IsValidPtr(entity) || entity < 0x100000000) continue;
                if (!IsValidPtr(entityType) || entityType < 0x100000000) continue;
                
                
                uintptr_t visualStateAddr = entity + 0x1C8;
                if (!IsValidPtr(visualStateAddr) || visualStateAddr < 0x100000000) continue;
                
                __try {
                    uintptr_t visualState = Read<uintptr_t>(visualStateAddr);
                    if (!IsValidPtr(visualState) || visualState == 0 || visualState < 0x100000000)
                    {
                        continue; 
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    continue; 
                }
                
                Vec3 itemPos = { 0, 0, 0 };
                int distance = 0;
                
                
                __try {
                    bool hasPos = GetEntityPosition(entity, itemPos);
                    
                    
                    if (!hasPos)
                    {
                        continue; 
                    }
                    
                    
                    if (IsNaN(itemPos.x) || IsNaN(itemPos.y) || IsNaN(itemPos.z))
                    {
                        continue; 
                    }
                    
                    if (hasPos)
                    {
                        Vec3 referencePos;
                        bool hasReference = false;
                        
                        if (g_LocalPlayerValid)
                        {
                            referencePos = g_LocalPlayerPos;
                            hasReference = true;
                        }
                        else if (g_CameraValid)
                        {
                            referencePos = g_CameraPos;
                            hasReference = true;
                        }
                        
                        if (hasReference)
                        {
                            distance = (int)Distance3D(referencePos, itemPos);
                            
                            if (g_LocalPlayerValid && distance > g_ItemDistance) continue;
                        }
                        else
                        {
                            distance = -1;
                        }
                    }
                    else
                    {
                        if (g_DebugCounter == 0 && totalDrawn < 5)
                        {
                            Log("Item skipped: no position found");
                        }
                        continue;
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    if (g_DebugCounter == 0)
                    {
                        DWORD exceptionCode = GetExceptionCode();
                        wsprintfA(buf, "item: EXCEPTION in GetEntityPosition for item: code=0x%X entity=0x%X%08X", 
                            exceptionCode, (DWORD)(entity >> 32), (DWORD)entity);
                        Log(buf);
                    }
                    continue;
                }
                
                
                char itemName[64];
                ZeroMem(itemName, 64);
                
                
                __try {
                    
                    uintptr_t typeNamePtrAddr = entityType + 0x70;
                    if (!IsValidPtr(typeNamePtrAddr) || typeNamePtrAddr < 0x100000000) 
                    {
                        
                    }
                    else
                    {
                        uintptr_t typeNamePtr = Read<uintptr_t>(typeNamePtrAddr);
                        if (IsValidPtr(typeNamePtr) && typeNamePtr > 0x100000000)
                        {
                            
                            uintptr_t typeNameLenAddr = typeNamePtr + 0x8;
                            if (IsValidPtr(typeNameLenAddr) && typeNameLenAddr > 0x100000000)
                            {
                                WORD typeNameLen = Read<WORD>(typeNameLenAddr);
                                if (typeNameLen > 0 && typeNameLen < 60)
                                {
                                    
                                    uintptr_t typeNameStringAddr = typeNamePtr + 0x10;
                                    if (IsValidPtr(typeNameStringAddr) && typeNameStringAddr > 0x100000000)
                                    {
                                        for (int j = 0; j < typeNameLen && j < 59; j++)
                                        {
                                            uintptr_t charAddr = typeNameStringAddr + j;
                                            if (!IsValidPtr(charAddr) || charAddr < 0x100000000) break;
                                            itemName[j] = Read<char>(charAddr);
                                        }
                                        itemName[typeNameLen] = 0;
                                    }
                                }
                            }
                        }
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    if (g_DebugCounter == 0)
                    {
                        Log("item exception typeName");
                    }
                }
                
                
                if (itemName[0] == 0)
                {
                    for (int j = 0; configName[j] && j < 60; j++)
                        itemName[j] = configName[j];
                }
                
                if (g_DebugCounter == 0 && totalDrawn < 5)
                {
                    wsprintfA(buf, "Drawing item: name='%s' distance=%d", itemName, distance);
                    Log(buf);
                }
                
                
                
                if (IsNaN(itemPos.x) || IsNaN(itemPos.y) || IsNaN(itemPos.z))
                {
                    continue; 
                }
                
                
                __try {
                    if (g_DebugCounter == 0)
                    {
                        wsprintfA(buf, "item: Calling DrawItem: entity=0x%X%08X name='%s' distance=%d pos=(%.1f, %.1f, %.1f)", 
                            (DWORD)(entity >> 32), (DWORD)entity, itemName, distance, itemPos.x, itemPos.y, itemPos.z);
                        Log(buf);
                    }
                    DrawItem(entity, itemName, distance);  
                    totalDrawn++;
                    if (g_DebugCounter == 0)
                    {
                        wsprintfA(buf, "item: DrawItem completed successfully");
                        Log(buf);
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    if (g_DebugCounter == 0)
                    {
                        DWORD exceptionCode = GetExceptionCode();
                        wsprintfA(buf, "item: EXCEPTION in DrawItem: code=0x%X entity=0x%X%08X", 
                            exceptionCode, (DWORD)(entity >> 32), (DWORD)entity);
                        Log(buf);
                    }
                    continue;
                }
            }
            else if (isVehicle)
            {
                
                Vec3 vehiclePos;
                int distance = 0;
                if (GetEntityPosition(entity, vehiclePos))
                {
                    Vec3 referencePos;
                    bool hasReference = false;
                    
                    if (g_LocalPlayerValid)
                    {
                        referencePos = g_LocalPlayerPos;
                        hasReference = true;
                    }
                    else if (g_CameraValid)
                    {
                        referencePos = g_CameraPos;
                        hasReference = true;
                    }
                    
                    if (hasReference)
                    {
                        distance = (int)Distance3D(referencePos, vehiclePos);
                        
                        if (g_LocalPlayerValid && distance > g_VehicleDistance) continue;
                    }
                    else
                    {
                        distance = -1;
                    }
                }
                
                
                char vehicleName[64];
                ZeroMem(vehicleName, 64);
                
                uintptr_t typeNamePtr = Read<uintptr_t>(entityType + 0x70);
                if (IsValidPtr(typeNamePtr) && typeNamePtr > 0x100000000)
                {
                    WORD typeNameLen = Read<WORD>(typeNamePtr + 0x8);
                    if (typeNameLen > 0 && typeNameLen < 60)
                    {
                        for (int j = 0; j < typeNameLen && j < 59; j++)
                        {
                            vehicleName[j] = Read<char>(typeNamePtr + 0x10 + j);
                        }
                        vehicleName[typeNameLen] = 0;
                    }
                }
                
                
                if (vehicleName[0] == 0)
                {
                    
                    if (StrCmp(configName, "car") == 0)
                    {
                        vehicleName[0] = 'C'; vehicleName[1] = 'a'; vehicleName[2] = 'r';
                        vehicleName[3] = 0;
                    }
                    else if (StrCmpI(configName, "boat") == 0)
                    {
                        vehicleName[0] = 'B'; vehicleName[1] = 'o'; vehicleName[2] = 'a'; vehicleName[3] = 't';
                        vehicleName[4] = 0;
                    }
                    else
                    {
                        for (int j = 0; configName[j] && j < 60; j++)
                            vehicleName[j] = configName[j];
                    }
                }
                
                
                DrawVehicle(entity, vehicleName, distance);
                totalDrawn++;
            }
        }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            if (g_DebugCounter == 0)
            {
                DWORD exceptionCode = GetExceptionCode();
                wsprintfA(buf, "item: EXCEPTION in entity loop: code=0x%X listIdx=%d listName='%s' entListPtr=0x%X%08X", 
                    exceptionCode, listIdx, lists[listIdx].name, (DWORD)(entListPtr >> 32), (DWORD)entListPtr);
                Log(buf);
            }
            
        }
    }
    
    if (g_DebugCounter == 0)
    {
        wsprintfA(buf, "drawn %d", totalDrawn);
        Log(buf);
        g_DebugCounter = 1;
    }
}


HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!g_Initialized)
    {
        Log("present hook init");
        
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_Device)))
        {
            g_Device->GetImmediateContext(&g_Context);
            g_SwapChain = pSwapChain;
            
            
            DXGI_SWAP_CHAIN_DESC desc;
            pSwapChain->GetDesc(&desc);
            g_ScreenWidth = (float)desc.BufferDesc.Width;
            g_ScreenHeight = (float)desc.BufferDesc.Height;
            
            
            g_NDCScaleX = 2.0f / g_ScreenWidth;
            g_NDCScaleY = 2.0f / g_ScreenHeight;
            g_ScreenHalfW = g_ScreenWidth / 2.0f;
            g_ScreenHalfH = g_ScreenHeight / 2.0f;
            
            char buf[128];
            wsprintfA(buf, "Screen: %.0f x %.0f", g_ScreenWidth, g_ScreenHeight);
            Log(buf);
            
            
            ID3D11Texture2D* backBuffer = NULL;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
            if (backBuffer)
            {
                g_Device->CreateRenderTargetView(backBuffer, NULL, &g_RenderTarget);
                backBuffer->Release();
            }
            
            
            InitRenderer();
            
            
            g_Initialized = true;
            Log("d3d ok");
        }
    }
    
    
    UpdateFromPanel();
    
    
    static int callCount = 0;
    if (++callCount % 1000 == 0)
    {
        char buf[128];
        wsprintfA(buf, "fb check %d", g_FastBullets);
        Log(buf);
    }
    if (g_FastBullets)
    {
        UpdateFastBullets();
        UpdateBulletAimbot();
    }
    
    g_FrameCount++;
    
    
    if (GetAsyncKeyState(VK_INSERT) & 1)
    {
        g_ShowESP = !g_ShowESP;
        char buf[64];
        wsprintfA(buf, "ESP: %s", g_ShowESP ? "ON" : "OFF");
        Log(buf);
    }
    
    
    if (GetAsyncKeyState(VK_HOME) & 1)
    {
        g_DebugCounter = 0;
        Log("debug reset");
    }
    
    
    if (GetAsyncKeyState(VK_DELETE) & 1)
    {
        g_ShowSkeleton = !g_ShowSkeleton;
        char buf[64];
        wsprintfA(buf, "skeleton %s", g_ShowSkeleton ? "on" : "off");
        Log(buf);
    }
    
    
    if (GetAsyncKeyState(VK_END) & 1)
    {
        Log("end pressed");
        g_Running = false;
    }
    
    
    if (g_RenderInit && g_ShowESP)
    {
        BeginDraw();
        
        
        RenderESP();
        
        EndDraw();
    }
    
    
    if (g_FrameCount % 1000 == 0)
    {
        char buf[256];
        wsprintfA(buf, "Frame %d - g_ShowNames=%d, g_ShowDistance=%d, g_ShowSkeleton=%d, g_ShowESP=%d, g_VertexCount=%d", 
            g_FrameCount, 
            g_ShowNames ? 1 : 0, 
            g_ShowDistance ? 1 : 0,
            g_ShowSkeleton ? 1 : 0,
            g_ShowESP ? 1 : 0,
            g_VertexCount);
        Log(buf);
    }
    
    return oPresent(pSwapChain, SyncInterval, Flags);
}


static HWND FindGameWindow()
{
    HWND hwnd = FindWindowA("DayZ", NULL);
    if (hwnd) return hwnd;
    hwnd = FindWindowA(NULL, "DayZ");
    if (hwnd) return hwnd;
    return GetForegroundWindow();
}

DWORD WINAPI MainThread(LPVOID lpParam)
{
    Log("main thread started");
    Sleep(5000);
    
    
    g_GameModule = GetModuleHandleA(NULL);
    if (!g_GameModule)
    {
        Log("no game module");
        return 1;
    }
    
    char buf[128];
    wsprintfA(buf, "Game module: 0x%p", g_GameModule);
    Log(buf);
    
    
    HWND gameWnd = FindGameWindow();
    if (!gameWnd)
    {
        Log("no game window");
        return 1;
    }
    
    wsprintfA(buf, "Found game window: 0x%p", gameWnd);
    Log(buf);
    
    
    Log("getting dxgi...");
    
    IDXGIFactory* pFactory = NULL;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    if (FAILED(hr) || !pFactory)
    {
        wsprintfA(buf, "CreateDXGIFactory failed: 0x%X", hr);
        Log(buf);
        return 1;
    }
    Log("got dxgi");
    
    
    IDXGIAdapter* pAdapter = NULL;
    hr = pFactory->EnumAdapters(0, &pAdapter);
    if (FAILED(hr) || !pAdapter)
    {
        Log("EnumAdapters failed");
        pFactory->Release();
        return 1;
    }
    Log("got adapter");
    
    
    ID3D11Device* pDevice = NULL;
    ID3D11DeviceContext* pContext = NULL;
    
    hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0,
        D3D11_SDK_VERSION, &pDevice, NULL, &pContext);
    
    if (FAILED(hr) || !pDevice)
    {
        wsprintfA(buf, "D3D11CreateDevice failed: 0x%X", hr);
        Log(buf);
        pAdapter->Release();
        pFactory->Release();
        return 1;
    }
    Log("d3d device ok");
    
    
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMem(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = gameWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    IDXGISwapChain* pSwapChain = NULL;
    hr = pFactory->CreateSwapChain(pDevice, &sd, &pSwapChain);
    
    if (FAILED(hr) || !pSwapChain)
    {
        wsprintfA(buf, "CreateSwapChain failed: 0x%X", hr);
        Log(buf);
        pContext->Release();
        pDevice->Release();
        pAdapter->Release();
        pFactory->Release();
        return 1;
    }
    Log("swap chain ok");
    
    
    g_SwapChainVTable = *(void***)pSwapChain;
    oPresent = (tPresent)g_SwapChainVTable[8];
    
    wsprintfA(buf, "Present at: 0x%p", oPresent);
    Log(buf);
    
    
    DWORD oldProtect;
    VirtualProtect(&g_SwapChainVTable[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
    g_SwapChainVTable[8] = (void*)hkPresent;
    VirtualProtect(&g_SwapChainVTable[8], sizeof(void*), oldProtect, &oldProtect);
    
    Log("present hooked");
    
    
    pSwapChain->Release();
    pContext->Release();
    pDevice->Release();
    pAdapter->Release();
    pFactory->Release();
    
    Log("hook installed");
    
    
    while (g_Running)
    {
        Sleep(100);
    }
    
    
    if (g_SwapChainVTable && oPresent)
    {
        VirtualProtect(&g_SwapChainVTable[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        g_SwapChainVTable[8] = (void*)oPresent;
        VirtualProtect(&g_SwapChainVTable[8], sizeof(void*), oldProtect, &oldProtect);
        Log("unhooked");
    }
    
    Log("Shutdown complete");
    return 0;
}


extern "C" __declspec(dllexport) BOOL WINAPI _DllMainCRTStartup(
    HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        Log("dll attach");
        g_Module = hModule;
        
        HANDLE hThread = CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
        if (hThread)
        {
            Log("main thread created");
            CloseHandle(hThread);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        g_Running = false;
        DisconnectFromPanel();
    }
    
    return TRUE;
}
