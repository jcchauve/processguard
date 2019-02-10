del *.tds
del *.bak
del *.obj
del *.~??
del ..\processguardsrc.zip
md compact
copy processguardgrab.exe compact
7za  a ..\processguardsrc  *.* -TZIP -x!*.exe -x!*.obj -x!*.txt -x!*.tds -x!*.dcu -x!*.chm -x!*.bak
del ..\processguardexe.zip
7za  a  ..\processguardexe -TZIP processguardgrab.exe
