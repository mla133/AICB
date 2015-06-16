#!/bin/bash
# Might need to be run as root

if [ -z "$1" ]; then
  echo usage: $0 query 
  exit
fi

QUERY=$1
DL_DIR=download/$QUERY
echo Building Query Structure...

echo "http://archive.org/advancedsearch.php?q=" > beginquery
echo "&fl[]=identifier&output=csv" > endquery
# alias urlencode='python -c "import sys, urllib as ul; print ul.quote_plus(sys.argv[1])"'
python -c "import sys, urllib as ul; print ul.quote_plus('$QUERY')" > querystring
cat beginquery querystring endquery | tr -d '\n' | sed '$a\' > iaquery

echo New link built...
cat iaquery

echo Fetching file list from Internet Archive.org...
wget -i iaquery -O iafilelist
cat iafilelist | tr -d [:punct:] | sed '1d' > iafilelist-clean
mkdir -p $DL_DIR
cd $DL_DIR
wget -r -H -nc -nd -np -nH --cut-dirs=2 -A .txt -e robots=off -l1 -i ../../iafilelist-clean -B 'http://archive.org/download/'

ls

## Need to test below for metadata and swish-E configuration

#mkdir metadata
#mv ?????.txt_meta.txt metadata/
#mv ?????-0.txt_meta.txt metadata/
#ls metadata
#rm *meta.txt
#rm pg*txt
#rm *-8.txt
#cd ..
#
# mkdir burstdocs
#cd burstdocs
#cp ../download/*.txt .
#ls
#for fileName in $(ls -1 *.txt) ; do split -d -a 4 -l 20 $fileName $fileName.x ; done
#rm ?????.txt ?????-0.txt
#rename 's/txt.//' *
#rename 's/$/.txt/' *

#echo Creating Swish-E conf file...
#echo "IndexDir burstdocs/" > swish.conf
#echo "IndexOnly .txt" >> swish.conf
#echo "IndexContents TXT* .txt" >> swish.conf
#echo "IndexFile ./burstdocs.index" >> swish.conf
#swish-e -c swish.conf
