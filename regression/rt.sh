# test Script

USAGE="Usage: `basename $0` DIRECTORY ASMFILE"

if [ $# -lt 2 ] ; then
  echo "$USAGE"
  exit 1
fi

# this is where the test will be performed
cd $1

# assemble
gpasm  $2.asm

# create the executable

../create_stc $2 temp.stc

../simulate temp.stc garbage.log
cat garbage.log
rm garbage.log
rm temp.stc
