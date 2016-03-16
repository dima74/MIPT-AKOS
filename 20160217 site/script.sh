#!/bin/sh -e
if [ -z $1 ]; then
#	host="ru.simplesite.com"
	host="cs.mipt.ru"
#	host="lectoriy.mipt.ru"
	echo "Вы забыли ввести сайт"
	echo "Используем $host"
else
	host=$1
fi

rm -rf site/
Save()
{
	file="site"$1
	case $file in
	     */) file=$file"index";;
	esac
	if [ -s "$file" ]; then
		#echo "(Уже существует)"
		return
	fi
	path=$(dirname "$file")	
	mkdir -p $path
	request="GET $1 HTTP/1.0\r\nHOST: $host\r\n\r\n"
	#echo $request
	echo -n "$request" | nc $host 80 >"$file"
	./extract-links.sh "$host" <"$file" >links

	# считаем число ссылок из этого файла (уникальные/все)
	cat links | \
	{ 
		real=0
		all=0
		while read line
		do
			file="site"$line
			case $file in
			     */) file=$file"index";;
			esac
			if [ ! -s "$file" ]; then
				real=$((real+1))
			fi
			all=$((all+1))
		done
		printf "%3d/%3d %s\n" $real $all $1
	}

	cat links | while read line
	do
		Save $line
	done
	echo "  done "$1
}

Save "/"
echo ""
# Условие должно выполниться если файла links не существует, но почему-то выполняется и когда он существует
if [ ! -s "links" ]; then
	#cat response
	echo "links not found"
fi
rm links