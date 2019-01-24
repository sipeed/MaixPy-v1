#!/bin/bash

TEMP=`getopt -o rt:d:h --long iw-long,ignore-warning-long,type-long:,dir-long:,help-long \
     -n 'insert_license.sh' -- "$@"`

if [ $? != 0 ] ; then echo "Terminating..." >&2 ; exit 1 ; fi
eval set -- "$TEMP"

if_Iw=false
if_Recursive=
dir_addr=
file_type=
declare -a ignore_dir_list
ignore_dir_list_file=ignore_dir
if_ignore_dir=false
while true ; do
    case "$1" in
        -r) if_Recursive=true ; shift ;;
        --iw-long|--ignore-warning-long) if_Iw=true ; shift ;;
        -t|--type-long) 
            if [ ! $file_type ]; then
                file_type=$2;
            else
                file_type=$file_type"|"$2;
            fi
        shift 2 ;;
        -d|--dir-long) dir_addr=$2; shift 2 ;;
        -h|--help-long) echo "Usage: Current_script_name [-f dir_addr] [-iw] [-r]";
            echo "./insert_license.sh --iw -tc --type cpp --ignore-warning -dtest_dir -r"
            echo "Add copyright information to the searched c, cpp, h, hpp type files";
            echo "Sort entries alphabetically if none of -cftuvSUX nor --sort is specified.";
            echo " ";
            echo "-iw,--ignore-warning    Ignore warning";
            echo "-d,--dir                Search for the directory you entered,";
            echo "                        If this parameter does not exist, the ";
            echo "                        default is the current directory of the script.";
            echo "-r                      Search the directory you need to search and ";
            echo "                        its subdirectories"
            echo "                        If this parameter does not exist, the default";
            echo "                        recursive search"
            echo "                        script current directory and all its subdirectories.";
            echo "-t,--type               This parameter defines the type of file that .";
            echo "                        needs to be modified"
            echo "                        If this parameter is not defined, it will search ";
            echo "                        for files of type c, cpp, h, hpp by default.";
        shift ;;
        --) shift ; break ;;
        *) echo "Internal error!" ; exit 1 ;;
    esac
done
# echo "Remaining arguments:"
for arg do
   echo '--> '"\`$arg'" ;
done
temp_i=0
if [ -f $ignore_dir_list_file ];then
	while read -r line
	do
	    ignore_dir_list[$temp_i]=${line};
            echo $temp_i":"${ignore_dir_list[$temp_i]};
            ((temp_i++))
	done < $ignore_dir_list_file
fi

echo "------------------------------------------------------------------"
if [ ! $file_type ]; then
    file_type="c|cpp|h|hpp";
fi

if [ ! $if_Recursive ]; then
    if_Recursive=false;
fi

# if [ $dir_addr ]; then
#     cd $dir_addr;
# fi


echo "file_type:$file_type";
shopt -s globstar nullglob extglob

function write_copyright(){
  first_line=`sed -n "1p" $1`
  # echo "first_line:$first_line"
  if ! grep -q Copyright $1
  then
      if_ignore_dir=false;
      for ignore_dir in ${ignore_dir_list[@]};do
	
        if [[ $1 =~ ^$ignore_dir ]];then echo -e "\033[36m ignore\033[0m:"$1;if_ignore_dir=true;break; fi
      done
      if ([[ $first_line =~ "/*" ]] && (! $if_Iw)) || $if_ignore_dir;
      then
	if ! $if_ignore_dir;then
	    echo -e "\033[31m Warning \033[0m:[$1]The first line of this file contains the commented string "
        fi
      else
            echo $1
            cat copyright.txt >$1.new && echo -e "\r\n" >> $1.new && cat $1 >> $1.new  && mv $1.new $1
      fi
  fi
}
if $if_Recursive;then
    for i in $dir_addr**/*.@($file_type);do
        # echo $i
        write_copyright $i
    done
else
    if [ $dir_addr ]; then
        $dir_addr="."
    fi
    for i in $dir_addr/*.@($file_type);do
        # echo $i
        write_copyright $i
    done
fi

# if [ $dir_addr ]; then
#     cd -;
# fi

