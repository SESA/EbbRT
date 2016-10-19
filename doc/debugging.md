1. Run Debug binary first and jot down the "gdb:..." number
2. Open emacs and run gdb on baremetal/build/debug/*.elf file i.e. gdb -i=mi
   SerializeEbb.elf
   3. M-x set-variable
   4. gdb-non-stop-setting to be nil
   5. quit gdb in emacs
   6. run gdb on elf again
   7. target remote localhost: gdb number
   8. The backend will now be stopped

