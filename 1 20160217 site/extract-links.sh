awk -v host="$1" '{
a = $0
while ((i = match(a, /<a href="([^\"]*)"/, arr)) != 0)
{
	a = substr(a, index(a, "</a>") + 4)
	b = arr[1]
	if (b == "" || b ~ "^#" || b ~ /^https/)
		break
	
	b = gensub(/%2F|%2f/, "/", "g", b)
	if (b ~ /^http/)
	{
		#1234567
		#http://
		b = substr(b, 8)
		if (substr(b, 1, length(host)) == host)
			b = substr(b, length(host) + 1)
		else
			continue
	}
	if (substr(b, 1, 1) != "/")
		b = "/" b
	if (substr(b, length(b)) != "/")
		b = b "/"
	print b
}
}'