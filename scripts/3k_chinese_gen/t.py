

chinese_str = "哈哦擦"

chinese_space = " ".join(chinese_str)

chinese_list = chinese_space.split(" ")
print("加空格 =>",  chinese_space)

print("转数组 =>", chinese_list)

print("转ord =>", [hex(ord(c)) for c in chinese_str])
print("转ord =>", " ".join([hex(ord(c)) for c in chinese_str]))
