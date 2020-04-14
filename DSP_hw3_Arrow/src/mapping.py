import sys

result = {}
from_file_path = sys.argv[1]
save_file_path = sys.argv[2]

# Read Big5-ZhuYin.map
with open(from_file_path,'r',encoding='big5-hkscs') as f:
	for line in f.readlines():
		ZhuYin_set = set()
		Big5 = line.strip('\n').split(" ")[0]
		ZhuYins = line.strip('\n').split(" ")[1].split("/")
		for ZhuYin in ZhuYins:
			ZhuYin_set.add(ZhuYin[0])

		for ZhuYin in list(ZhuYin_set):
			if(ZhuYin in result.keys()):
				result[ZhuYin].append(Big5)
			else:
				result[ZhuYin] = [Big5]
			result[Big5] = Big5

# Write ZhuYin-Big5.map
with open(save_file_path,'w',encoding='big5-hkscs') as f:
	for z in result:
		f.write(z+' ') #修正改為空白
		for b in result[z]:
			f.write(b+' ')
		f.write('\n')
