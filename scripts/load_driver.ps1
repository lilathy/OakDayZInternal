$driverPath = "C:\Program Files\GIGABYTE\Control Center\Lib\MBStorage\MyPortIO_x64.sys"
$serviceName = "MyPortIO"


sc.exe delete $serviceName 2>$null


sc.exe create $serviceName type= kernel binPath= $driverPath
sc.exe start $serviceName


sc.exe query $serviceName
