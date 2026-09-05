count=$#
sum=0

for arg in "$@"; do
  sum=$((sum + arg))
done

result=$((sum/count))

echo "аргументов: $count"
echo "среднее: $result"
