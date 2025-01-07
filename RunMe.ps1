function GetProjectName {
    $project = Read-Host "Enter your new project name"

    # Check for invalid characters in the project name
    if ($project -match "[^a-zA-Z0-9]") {
        Write-Host "The project name contains special characters or spaces. Please enter a valid project name."
        return GetProjectName  # Recurse if invalid name
    }

    return $project
}

$project = GetProjectName

# Ensure the correct file paths are used when renaming items
$projectPath = "$PWD\$project"  # Full path to the project directory
$sourceDirectory = "$PWD"  # Current directory of the script

# Rename files based on the project name
Rename-Item "$sourceDirectory\CarbonTemplate.sln" "$sourceDirectory\$project.sln"
Rename-Item "$sourceDirectory\CarbonTemplate" "$sourceDirectory\$project"
Rename-Item "$sourceDirectory\$project\CarbonTemplate.vcxproj" "$sourceDirectory\$project\$project.vcxproj"

# Function to replace text in a file
function Replace-TextInFile {
    param (
        [string]$filePath,
        [string]$search,
        [string]$replace
    )

    # Ensure the file exists before proceeding
    if (Test-Path $filePath) {
        # Read the file, replace text, and write the result to a temporary file
        $content = Get-Content $filePath
        $content = $content -replace $search, $replace
        $content | Set-Content "$filePath.tmp"

        # Delete the original file and rename the temporary file
        Remove-Item $filePath
        Rename-Item "$filePath.tmp" $filePath
    } else {
        Write-Host "File $filePath not found, skipping replacement."
    }
}

# Replace text in the solution and project files
Replace-TextInFile "$sourceDirectory\$project.sln" "CarbonTemplate" $project
Replace-TextInFile "$sourceDirectory\$project\$project.vcxproj" "CarbonTemplate" $project

# Prompt to remove the RunMe.ps1 script
$input = Read-Host -Prompt "Do you want to remove the RunMe.ps1 script? (it's no longer needed) (Y/N)"
if ($input -eq "Y") {
    Remove-Item "$sourceDirectory\RunMe.ps1"
}

# Prompt to open the project in Visual Studio
$input = Read-Host -Prompt "Do you want to open the project in Visual Studio? (Y/N)"
if ($input -eq "Y") {
    start "$sourceDirectory\$project.sln"
}
