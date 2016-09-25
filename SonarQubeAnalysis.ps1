
Param(
	[parameter(Mandatory=$true)]
	[alias("h")]
	[string]$hostUrl, # Server URL (see http://docs.sonarqube.org/display/SONAR/Analysis+Parameters)
	[parameter(Mandatory=$true)]
	[alias("l")]
	[string]$login,
	[parameter(Mandatory=$true)]
	[alias("n")]
	[string]$projectName, #Name of the project that will be displayed on the web interface.Set through <name> when using Maven.
	[parameter(Mandatory=$true)]
	[alias("k")]
	[string]$projectKey, #The project key that is unique for each project. Allowed characters are: letters, numbers, '-', '_', '.' and ':', with at least one non-digit. When using Maven, it is automatically set to <groupId>:<artifactId>.
	[parameter(Mandatory=$true)]
	[alias("v")]
	[string]$projectVersion, # The project version. Set through <version> when using Maven.
	[parameter(Mandatory=$true)]
	[alias("s")]
	[string]$sources, #Comma-separated paths to directories containing source files. Compatible with Maven. If not set, the source code is retrieved from the default Maven source code location. 
	#Option build wrapper command (for C/C++/Objective-C builds)
	[string]$buildWrapperCommand,
	# Pull request specific arguments
	[int]$gitHubPullRequest, #Pull request number
	[string]$gitHubOauth,  #Personal access token generated in GitHub for the technical user (see http://docs.sonarqube.org/display/PLUG/GitHub+Plugin)	
	[string]$gitHubRepository	#Identification of the repository. Format is: <organisation/repo>. Exemple: SonarSource/sonarqube	
)

Add-Type -assembly system.io.compression.filesystem

# Download and unzip sonnar scanner
if(![System.IO.Directory]::Exists($PSScriptRoot + '\SonarScanner'))
{
	(new-object net.webclient).DownloadFile('http://repo1.maven.org/maven2/org/sonarsource/scanner/cli/sonar-scanner-cli/2.6/sonar-scanner-cli-2.6.zip', $PSScriptRoot +  '\SonarScanner.zip')
	[io.compression.zipfile]::ExtractToDirectory($PSScriptRoot + '\SonarScanner.zip', $PSScriptRoot + '\SonarScanner')
}

$scannerCmdLine = ".\SonarScanner\sonar-scanner-2.6\bin\sonar-scanner.bat -D sonar.host.url='$hostUrl' -D sonar.login='$login' -D sonar.projectKey='$projectKey' -D sonar.projectName='$projectName' -D sonar.projectVersion='$projectVersion' -D sonar.sources='$sources'"

#Download build wrapper (if needed)
if($buildWrapperCommand)
{
	if(![System.IO.Directory]::Exists($PSScriptRoot +  '\BuildWrapper'))
	{
		[System.Net.ServicePointManager]::SecurityProtocol = @('Tls12','Tls11','Tls','Ssl3')
		(new-object net.webclient).DownloadFile('http://sonarqube.com/static/cpp/build-wrapper-win-x86.zip', $PSScriptRoot + '\BuildWrapper.zip')
		[io.compression.zipfile]::ExtractToDirectory($PSScriptRoot + '\BuildWrapper.zip', $PSScriptRoot + '\BuildWrapper')
	}
	
	# Compile with BuildWrapper
	
    $builderCmdLine = ".\BuildWrapper\build-wrapper-win-x86\build-wrapper-win-x86-64.exe --out-dir 'Build' $buildWrapperCommand"
	Write-Output $builderCmdLine
	Invoke-Expression $builderCmdLine
	
	$scannerCmdLine += ' -D sonar.cfamily.build-wrapper-output=Build'
}

# Pull request ?
if($gitHubPullRequest)
{
	$scannerCmdLine += " -D sonar.analysis.mode=preview -D sonar.github.oauth='$gitHubOauth' -D sonar.github.repository='$gitHubRepository' -D sonar.github.pullRequest='$gitHubPullRequest'"
}

Write-Output $scannerCmdLine

Invoke-Expression $scannerCmdLine
