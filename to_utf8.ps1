Get-ChildItem .\src -Recurse -Include *.cpp |
	Rename-Item -NewName {
		$_.FullName + ".bkup"
	}
Get-ChildItem .\src -Recurse -Include *.cpp.bkup |
	ForEach-Object {
		Get-Content $_.FullName |
			Out-File -Encoding UTF8 ($_.FullName -replace '.cpp.bkup','.cpp')
			Remove-Item $_.FullName
	}

Get-ChildItem .\src -Recurse -Include *.h |
	Rename-Item -NewName {
		$_.FullName + ".bkup"
	}
Get-ChildItem .\src -Recurse -Include *.h.bkup |
	ForEach-Object {
		Get-Content $_.FullName |
			Out-File -Encoding UTF8 ($_.FullName -replace '.h.bkup','.h')
			Remove-Item $_.FullName
	}

Get-ChildItem .\gtests -Recurse -Include *.cpp |
	Rename-Item -NewName {
		$_.FullName + ".bkup"
	}
Get-ChildItem .\gtests -Recurse -Include *.cpp.bkup |
	ForEach-Object {
		Get-Content $_.FullName |
			Out-File -Encoding UTF8 ($_.FullName -replace '.cpp.bkup','.cpp')
			Remove-Item $_.FullName
	}

Get-ChildItem .\gtests -Recurse -Include *.h |
	Rename-Item -NewName {
		$_.FullName + ".bkup"
	}
Get-ChildItem .\gtests -Recurse -Include *.h.bkup |
	ForEach-Object {
		Get-Content $_.FullName |
			Out-File -Encoding UTF8 ($_.FullName -replace '.h.bkup','.h')
			Remove-Item $_.FullName
	}
