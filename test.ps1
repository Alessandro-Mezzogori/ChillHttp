# Building project
msbuild .\ChillHttp.sln /t:ChillHttp:Rebuild /p:Configuration=Release /p:Platform="x64"

# Running test server with files
Start-Process -FilePath ".\x64\Release\ChillHttp.exe" -WorkingDirectory ".\x64\Release"

# Running tests
Start-Sleep -Seconds 1
dotnet test Tests
