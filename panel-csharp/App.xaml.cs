using System;
using System.Threading;
using System.Windows;

namespace OakPanel
{
    public partial class App : Application
    {
        private static Mutex? _mutex = null;
        private const string MutexName = "OakPanel_SingleInstance_Mutex";

        protected override void OnStartup(StartupEventArgs e)
        {
            
            bool createdNew;
            _mutex = new Mutex(true, MutexName, out createdNew);

            if (!createdNew)
            {
                
                MessageBox.Show(
                    "panel already running.\n\nclose the other one first.",
                    "oak panel",
                    MessageBoxButton.OK,
                    MessageBoxImage.Information);
                Shutdown();
                return;
            }

            base.OnStartup(e);
        }

        protected override void OnExit(ExitEventArgs e)
        {
            _mutex?.ReleaseMutex();
            _mutex?.Dispose();
            base.OnExit(e);
        }
    }
}
