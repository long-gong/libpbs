# CMake generated Testfile for 
# Source directory: /media/saber/easystore/libpbs/src
# Build directory: /media/saber/easystore/libpbs/build-new/src
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(vtest "valgrind" "--leak-check=yes" "--error-exitcode=1" "/media/saber/easystore/libpbs/build-new/src/test_pbs_messages")
