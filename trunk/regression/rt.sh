# test Script

USAGE="Usage: `basename $0` BASENAME"

if [ $# -lt 1 ] ; then
  echo "$USAGE"
  exit 1
fi

# assemble
gpasm  $1.asm

# create the executable

./create_stc $1 temp.stc

./simulate temp.stc garbage.log
cat garbage.log
rm garbage.log
