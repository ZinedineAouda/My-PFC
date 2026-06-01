# Fix Hibernation and Ubuntu Boot Entry
# Run this script as Administrator

Write-Output "======================================================="
Write-Output "Starting PC Fix Script: Hibernation & UEFI Cleanup"
Write-Output "======================================================="
Write-Output ""

# 1. Check for Admin Privileges
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Warning "CRITICAL ERROR: This script MUST be run as an Administrator."
    Write-Warning "Please close this window, right-click the script (or PowerShell), and select 'Run as administrator'."
    Read-Host "Press Enter to exit"
    exit
}

# 2. Re-enable System-Managed Paging File (Virtual RAM)
Write-Output "[1/4] Re-enabling Virtual RAM (System-Managed Paging File)..."
try {
    $ComputerSystem = Get-CimInstance Win32_ComputerSystem
    if (-not $ComputerSystem.AutomaticManagedPagefile) {
        $ComputerSystem.AutomaticManagedPagefile = $true
        Set-CimInstance -CimInstance $ComputerSystem
        Write-Output "-> Virtual RAM has been successfully set to automatically manage pagefile size."
    } else {
        Write-Output "-> Virtual RAM was already set to automatically manage pagefile size."
    }
} catch {
    Write-Warning "Could not update Virtual RAM: $_"
}
Write-Output ""

# 3. Enable Hibernation & recreate hiberfil.sys
Write-Output "[2/4] Re-enabling Hibernation..."
try {
    powercfg /h on
    powercfg /h /type full
    Write-Output "-> Hibernation and the hiberfil.sys file have been successfully enabled!"
} catch {
    Write-Warning "Could not enable Hibernation: $_"
}
Write-Output ""

# 4. Remove EFI\ubuntu directory from the EFI system partition
Write-Output "[3/4] Mounting EFI Partition to remove Ubuntu folder..."
try {
    # UEFI system partition type GUID
    $efiPartition = Get-Partition | Where-Object { $_.GptType -eq "{c12a7328-f81f-11d2-ba4b-00a0c93ec93b}" }
    if ($efiPartition) {
        $driveLetter = "Z"
        # Try to assign drive letter Z: to mount the EFI partition
        Set-Partition -DiskNumber $efiPartition.DiskNumber -PartitionNumber $efiPartition.PartitionNumber -NewDriveLetter $driveLetter -ErrorAction SilentlyContinue
        Start-Sleep -Seconds 2

        if (Test-Path "Z:\EFI\ubuntu") {
            Remove-Item -Path "Z:\EFI\ubuntu" -Recurse -Force -ErrorAction Stop
            Write-Output "-> Successfully deleted Z:\EFI\ubuntu folder!"
        } else {
            Write-Output "-> The EFI\ubuntu folder does not exist (already deleted)."
        }

        # Dismount the EFI partition
        Remove-PartitionAccessPath -DiskNumber $efiPartition.DiskNumber -PartitionNumber $efiPartition.PartitionNumber -AccessPath "Z:\" -ErrorAction SilentlyContinue
        Write-Output "-> Successfully unmounted the EFI partition."
    } else {
        Write-Warning "Could not locate the EFI System Partition."
    }
} catch {
    Write-Warning "Could not complete EFI folder cleanup: $_"
    # Ensure unmount in case of failure
    if ($efiPartition) {
        Remove-PartitionAccessPath -DiskNumber $efiPartition.DiskNumber -PartitionNumber $efiPartition.PartitionNumber -AccessPath "Z:\" -ErrorAction SilentlyContinue
    }
}
Write-Output ""

# 5. Remove UEFI boot entry from NVRAM using bcdedit
Write-Output "[4/4] Removing Ubuntu UEFI boot entry from BIOS/NVRAM..."
try {
    $bcd = bcdedit /enum firmware
    $targetId = $null
    for ($i = 0; $i -lt $bcd.Length; $i++) {
        if ($bcd[$i] -match "identifier\s+(\{[a-fA-F0-9-]+\})") {
            $currentId = $Matches[1]
            # Check the following lines for ubuntu indicators
            for ($j = 1; $j -le 6; $j++) {
                if (($i + $j) -lt $bcd.Length -and ($bcd[$i + $j] -match "ubuntu" -or $bcd[$i + $j] -match "\\EFI\\ubuntu")) {
                    $targetId = $currentId
                    break
                }
            }
        }
        if ($targetId) { break }
    }

    if ($targetId) {
        bcdedit /delete $targetId
        Write-Output "-> Successfully deleted the UEFI entry for Ubuntu ($targetId)!"
    } else {
        Write-Output "-> No Ubuntu boot entry found in the UEFI configuration."
    }
} catch {
    Write-Warning "Could not remove UEFI boot entry: $_"
}
Write-Output ""

Write-Output "======================================================="
Write-Output "All operations complete!"
Write-Output "Please restart your computer for all changes to apply."
Write-Output "======================================================="
Write-Output ""
Read-Host "Press Enter to exit"
