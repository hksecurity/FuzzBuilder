import os, sys, subprocess

def log(prefix, msg):
    print ("[" + prefix + "] " + msg)

def usage():
    log ("INFO", "python " + os.path.basename(__file__) + " ${apis.txt} ${executable} ${seed} ...")

def get_gcda_files(src):
    ret = []
    for root, subdirs, files in os.walk(src):
        for file in files:
            if file.endswith(".gcda"):
                ret.append(root + os.sep + file)
    return ret

def option():
    ret = {}

    argv = sys.argv

    if len(argv) < 6:
        usage()
        sys.exit(0)

    api = argv[1]
    workspace = argv[2]
    source = argv[3]
    output = argv[4]
    type = argv[5]
    execs = []

    i = 6
    while i < len(argv):
        if type == "fuzz":
            execs.append([argv[i], argv[i+1]])
            i += 2
        else:
            execs.append([argv[i],])
            i += 1

    return api, workspace, source, output, execs

def create(src):
    f = open(src, "rt")
    contents = f.read().split("\n")
    f.close()

    ret = {}
    for e in contents:
        if len(e) == 0:
            continue
        ret[e] = 0

    return ret

def unique_path(prefix, extension):
    org = prefix + "." + extension
    cnt = 1
    while os.path.exists(org):
        org = prefix + "-" + str(cnt) + "." + extension
        cnt += 1
    return org

def get_function_coverage(src):
    ret = {}
    os.system("llvm-cov-6.0 gcov -n -f " + src + " > temp.cov 2> /dev/null")
    f = open("temp.cov")
    lines = f.read().split("\n")
    f.close()
    os.unlink("temp.cov")

    i = 0
    while i < len(lines):
        line = lines[i]
        if not line.startswith("Function"):
            i += 1
            continue

        fname = line.split(" ")[1]
        if not fname.startswith("'") or not fname.endswith("'"):
            log("ERROR", "Not expected gcno(" + src + ")")
            sys.exit(0)
        fname = fname[1:-1]

        if fname in ret:
            log("ERROR", "Not expected gcda(" + src + ")")
            sys.exit(0)

        nxt = lines[i+1]
        if not nxt.startswith("Lines executed:"):
            log("ERROR", "Not expected gcno(" + src + ")")
            sys.exit(0)

        nxt = nxt[len("Lines executed:"):]  
        tok = nxt.split("of")
        if len(tok) != 2:
            log("ERROR", "Not expected line(" + nxt + ")")
            sys.exit(0)

        percent = tok[0].strip()
        if not percent.endswith("%"):
            log("ERROR", "Not exepcted line(" + nxt + ")")
        percent = percent[:-1]

        total = tok[1].strip()
        line = int((float(percent) / 100.0) * float(total))

        ret[fname] = line
        i += 2

    return ret

def update(dst, src):
    for key in src:
        if key in dst:
            if src[key] > dst[key]:
                dst[key] = src[key]

    return dst 

def get_function_coverages(api, srcs):
    ret = api
    for src in srcs:
        funcs = get_function_coverage(src)
        ret = update(ret, funcs)
        
    return api

def execute_single(executable, run_type, seed, ws):
    cmd = [executable]
    output = open(os.devnull, "wt")
    if run_type == "fuzz":
        input = open(seed, "rt")
        print (" ".join(cmd) + " < " + seed)
        p = subprocess.Popen(cmd, stdout = output, stderr = output, stdin = input, cwd = ws).communicate()
        input.close()
    else:
        print (" ".join(cmd))
        print ("ws : " + ws)
        p = subprocess.Popen(cmd, stdout = output, stderr = output, cwd = ws).communicate()
    output.close()

def process_api(api, workspace, source, execs, output):
    ret = create(api)

    source = workspace + os.sep + source

    gcda_files = get_gcda_files(source)
    for gcda_file in gcda_files:
        os.unlink(gcda_file)

    for e in execs:
        if len(e) == 2:
            executable = workspace + os.sep + e[0]
            seed_dir = workspace + os.sep + e[1]

            seeds = []
            for e in os.listdir(seed_dir):
                if os.path.isdir(seed_dir + os.sep + e):
                    continue
                seeds.append(seed_dir + os.sep + e)

            for j in range(0, len(seeds)):
                seed = seeds[j]
                execute_single(executable, "fuzz", seed, source)
                print (os.path.basename(executable) + " : [" + str(j) + "/" + str(len(seeds)) + "]")
        else:
            executable = workspace + os.sep + e[0]
            execute_single(executable, "unit", "unit", source)
            print (os.path.basename(executable))

    gcda_files = get_gcda_files(source)
    gunc_covs = get_function_coverages(ret, gcda_files)
    report(output, gunc_covs)

def report(output, src):
    f = open(output, "wt")

    f.write("total functions : " + str(len(src)) + "\n")

    called = []
    ncalled = []
    for e in src:
        if src[e] > 0:
            called.append((e, src[e]))
        else:
            ncalled.append(e)

    f.write("called functions : " + str(len(called)) + "\n")

    f.write("called functions:\n")
    for e in called:
        f.write("\t" + e[0] + " : " + str(e[1]) + "\n")

    f.write("non called functions:\n")
    for e in ncalled:
        f.write("\t" + e + "\n")

    f.close()

def main():
    api, workspace, source, output, execs = option()
    process_api(api, workspace, source, execs, output)

if __name__ == "__main__":
    main()
