robocopy .\build32\rundir\%build_config% .\build\ /E /XF .gitignore
robocopy .\build64\rundir\%build_config% .\build\ /E /XC /XN /XO /XF .gitignore
7z a build.zip .\build\*
