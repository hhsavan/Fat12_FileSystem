#!/bin/bash

execute_with_delay() {
    commands=(
    	"./makeFileSystem 1 mySystem.dat"
        "./fileSystemOper mySystem.dat mkdir /a"
        "./fileSystemOper mySystem.dat mkdir /a/c"
        "./fileSystemOper mySystem.dat mkdir /b/c"
        "./fileSystemOper mySystem.dat write /a/c/f1 linuxFile.txt"
        "./fileSystemOper mySystem.dat write /a/f2 linuxFile.txt"
        "./fileSystemOper mySystem.dat write /f3 linuxFile.txt"
        "./fileSystemOper mySystem.dat dir /"
        "./fileSystemOper mySystem.dat del /a/c/f1"
        "./fileSystemOper mySystem.dat dumpe2fs"
        "./fileSystemOper mySystem.dat read /a/f2 linuxFile2.txt"
        "./fileSystemOper mySystem.dat chmod /a/f2 -rw"
        "./fileSystemOper mySystem.dat read /a/f2 linuxFile2.txt"
        "./fileSystemOper mySystem.dat chmod /a/f2 +rw"
        "./fileSystemOper mySystem.dat addpw /a/f2 1234"
        "./fileSystemOper mySystem.dat read /a/f2 linuxFile2.txt"
        "./fileSystemOper mySystem.dat read /a/f2 linuxFile2.txt 1234"
    )

    for cmd in "${commands[@]}"; do
        echo ""
        echo ">>: $cmd"
        eval $cmd
        sleep 1
    done
}

execute_with_delay
