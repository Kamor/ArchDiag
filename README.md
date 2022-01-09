# ArchDiag
Diagnose tool for the arch data from www.daimonin.org

This is my idea of cleaning and sorting the arch data. I also learned a lot of coding and had some fun.

## v0.1
- console command interface
- command h = show commands
- command p = set path (primary path = absolut)
- command a = set archpath (secondary path = relativ)
- command i = init (reinit path, arch and proto definitions, when you made changes from outside)
- command s = simulate (this helps finding errors in code or in data, it works readonly to files)
- command b = backup (make backup first of all .arc files)
- command d = delete (delete the backup files)
- command S = Sort (clean and sort all .arc files)
- command R = Restore (copy backup files to .arc files)

path.ini, arch.ini and proto.arc can be changed from outside
proto.arc defines the order, how Sort will sort the .arc files
archdiag.log is logfile for errors
