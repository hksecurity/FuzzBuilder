#!/usr/bin/env python
import sys, os

def usage():
    print ("[usage] python " + __file__ + " $seed_path $seed_out_path")

def main():
    if len(sys.argv) != 3:
        sys.exit(0)

    seed_path = sys.argv[1]
    seed_out_path = sys.argv[2]

    if not os.path.exists(seed_out_path):
        os.makedirs(seed_out_path)

    f = open(seed_path, "r")
    contents = f.read().split("\nfuzzbuilder=============\n")
    f.close()

    cnt_table = {}
    for i in range(0, len(contents)):
        print ("[" + str(i) + "/" + str(len(contents)) + "]")
        if len(contents[i]) == 0:
            continue
        content = contents[i]
        key = content[:content.find("\n")]
        seed = content[content.find("\n")+1:]

        cnt = 1
        cache = []
        try:
            cnt = cnt_table[key][0]
            cache = cnt_table[key][1]
        except KeyError:
            pass

        if seed in cache:
            continue

        try:
            if not os.path.exists(seed_out_path + os.sep + key):
                os.makedirs(seed_out_path + os.sep + key)
            f = open(seed_out_path + os.sep + key + os.sep + key + ".seed." + str(cnt), "w")
            f.write(seed)
            f.close()
        except TypeError:
            print ("key : " + key)

        cnt += 1
        cache.append(seed)
        cnt_table[key] = (cnt, cache)

if __name__ == "__main__":
    main()
