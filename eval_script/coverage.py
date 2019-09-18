import os, json, sys, csv, subprocess

def usage():
    print ("python " + __file__ + " worksapace id fuzzer source output, evaltype, execinfo...")
    sys.exit(0)

def get_option(argv):
    if len(argv) < 6:
        usage()

    workspace = argv[1]
    id = argv[2]
    fuzzer = argv[3]
    source = argv[4]
    evaltype = argv[5]
    output = argv[6]

    execs = []
    i = 7
    while i < len(argv):
        execs.append((argv[i], argv[i+1]))
        i += 2

    return workspace, id, fuzzer, source, evaltype, output, execs

class gcov_unit():
    def __init__(self, title, vl, vb, tl, tb):
        self.title = title
        self.vl = vl
        self.vb = vb
        self.tl = tl
        self.tb = tb
        self.file = ""
        self.line = 0
        self.endline = 0

    def getVisitedLine(self):
        if self.tl == 0:
            return 0
        else:
            return int(float(self.vl) / 100.0 * float(self.tl))

    def getVisitedBranch(self):
        if self.tb == 0:
            return 0
        else:
            return int(float(self.vb) / 100.0 * float(self.tb))

    def isVisited(self):
        if self.vl == 0.0 and self.vb == 0.0:
            return False
        return True

    def get_key(self):
        return self.file + " " + self.title

    def __str__(self):
        msg = self.title + "(" + self.file + ":" + str(self.line) + "~" + str(self.endline) + ") : "
        msg += "[lines " + str(self.vl) + "% of " + str(self.tl) + "],"
        msg += "[branches " + str(self.vb) + "% of " + str(self.tb) + "]"
        return msg

curdir=os.getcwd()

def get_gcov_source(src):

#    print ("[I] Analyze gcno file(" + src + ")")

    temp = getUniqueName("gcov", "tmp")
    os.system("llvm-cov gcov -f -b " + src + " > " + temp)
    tmp = parse_gcov(temp)
    gcovs = parse_gcov_for_files(temp)
    ret = {}
    for key in tmp:
        unit = tmp[key]
        unit.file = os.path.abspath(src)
        ret[unit.title] = unit
    os.remove(temp)

#    for gcov in gcovs:
#        print ("\t[I] Getting gcov sources (" + gcov + ")")
#    print ("\t[I] Getting " + str(len(tmp)) + " Functions\n")

    return ret, gcovs

def getUniqueName(prefix, extension):
    org = prefix + "." + extension
    cnt = 1
    while os.path.exists(org):
        org = prefix + "-" + str(cnt) + "." + extension
        cnt += 1
    return org

def parse_gcov_for_files(src):
    lines = None
    with open(src, "rt") as f:
        lines = f.read().split("\n")

    ret = []
    for i in range(0, len(lines)):
        if lines[i].startswith("File"):
            ret.append(lines[i].split()[-1][1:-1])

    return ret

def parse_gcov(src):
    lines = None
    with open(src, "rt") as f:
        lines = f.read().split("\n")

    db = {}
    for i in range(0, len(lines)):
        if lines[i].startswith("Function"):
            title = lines[i].split()[1][1:-1]
            vl = 0.00
            vb = 0.00
            tl = 0
            tb = 0
            if lines[i+1].startswith("Lines executed"):
                tl = int(lines[i+1].split()[-1])
                vl = float(lines[i+1][lines[i+1].find(":")+1:lines[i+1].find("%")])
            if lines[i+2].startswith("Branches executed"):
                tb = int(lines[i+2].split()[-1])
                vb = float(lines[i+2][lines[i+2].find(":")+1:lines[i+2].find("%")])
            unit = gcov_unit(title, vl, vb, tl, tb)

            #I assumed that each function names are uniquely distinguished in one gcno file.
            #So below is a defense code for exception case which has not seen before.
            if unit.title in db:
                raise RuntimeError("Unexpected case (" + src + ")")
            db[unit.title] = unit
    return db

def get_gcov_gcov(src):
    ret = {}
    try:
        ret = parse_gcov_file(src)
    except IOError:
        return ret

    os.remove(src)
    return ret

def parse_gcov_file(gcov_file):
    ret = {}

    file = open(gcov_file, "rt")
    contents = file.read().split("\n")

    if len(contents) == 0:
        print (gcov_file + " has unexpected format")
        raise RuntimeError("")

    if not "Source" in contents[0]:
        print (gcov_file + " has unexpected format")
        raise RuntimeError("")

    file = contents[0].split()[-1]
    file = contents[0][contents[0].rfind(":")+1:]
    file = os.path.abspath(file)

    contents = contents[1:]

    for i in reversed(range(0, len(contents))):
        content = contents[i].strip()
        if len(content) == 0:
            contents.pop(i)
            continue
        if content.startswith("branch"):
            contents.pop(i)
            continue

    functions = set()

    blocks = []
    c = 0
    i = 0
    while i < len(contents):
        content = contents[i].strip()
        if content.startswith("function"):
            if i > 0:
                blocks.append(contents[c:i])
            c = i
            while i < len(contents):
                content = contents[i].strip()
                if not content.startswith("function"):
                    break
                i += 1
        i += 1
    if c != i:
        blocks.append(contents[c:i])

    for block in blocks:
        if len(block) == 0:
            continue

        if not block[0].startswith("function"):
            continue

        funcs = set()
        sidx = 0
        eidx = 0
        i = 0
        while i < len(block):
            if block[i].startswith("function"):
                funcs.add(block[i].split()[1])
            else:
                sidx = block[i][block[i].find(":")+1:].strip()
                sidx = sidx[:sidx.find(":")]
                eidx = block[-1][block[-1].find(":")+1:].strip()
                eidx = eidx[:eidx.find(":")]
                break
            i += 1
        for func in funcs:
            ret[file + " " + func] = [int(sidx), int(eidx)]
    return ret

#
# Analyzing gcov file to get each functions start and end line
#
def analyze_gcov(tb, src):

#    print ("[I] Analyze gcov file(" + src + ")")

    tmp = get_gcov_gcov(src)

    for key in tmp:
        gcno_key = key.split()[-1]
        unit = tb[gcno_key]
        unit.line = int(tmp[key][0])
        unit.endline = int(tmp[key][1])
        del tb[gcno_key]
        tb[key] = unit

#        print ("\t[I] " + key.split()[1] + "(" + key.split()[0] + ") was updated to (" + str(unit.line) + ":" + str(unit.endline) + ")")

    return tb

def create(src):
    #1. Generate Table And Get Member Files Based On The Information Of GCNO File
    ret, srcs = get_gcov_source(src)

    for src in srcs:
        ret = analyze_gcov(ret, src + ".gcov")

    return ret

def get_seeds(src):
    seeds = []
    for file in os.listdir(src):
        if os.path.isdir(file):
            continue
        seeds.append(src + os.sep + file)

    return seeds

def evaluate(executable, seeds, targets):


    sys.exit(0)
    ret = []

    os.system("rm $(find ./* -name \"*.gcda\")")

    units = {}
    for target in targets:
        db = create(target)
        for key in db:
            units[key] = db[key]
    vl, vb, vf = get_visited_coverage(units)
    ret.append(["init", vl, vb, vf, len(initials)])
    last_coverage = vl
    last_branch = vb
    last_function = vf

    cnt = 1
    for seed in seeds:
        print ("Execute generated seeds (" + str(cnt) + "/" + str(len(seeds)) + ")")
        subprocess.Popen([executable, seed]).communicate()
        cnt += 1

    units = {}
    for target in targets:
        db = create(target)
        for key in db:
            units[key] = db[key]
    vl, vb, vf = get_visited_coverage(units)
    ret.append(["fuzz", vl, vb, vf, len(seeds)])
    last_coverage = vl
    last_branch = vb
    last_function = vf

    return ret

def get_visited_coverage(units):
    vl = 0
    vb = 0
    vf = 0

    for key in units:
        unit = units[key]
        vl += unit.getVisitedLine()
        vb += unit.getVisitedBranch()
        if unit.isVisited():
            vf += 1

    return vl, vb, vf

def get_visited_file(units):
    ret = set()

    for key in units:
        ret.add(key)

    return ret

def get_total_coverage(units):
    tl = 0
    tb = 0
    tf = 0

    for key in units:
        unit = units[key]
        tl += unit.tl
        tb += unit.tb
        tf += 1

    return tl, tb, tf

def report_exec(output, id, result, accept, reject, seeds):
    f = open(output, "wt")

    f.write("(" + id + ") [exec]\n")
    f.write("time line\n")

    for i in range(0, len(result)):
        f.write(str(float(float(result[i][0]) / 3600.0)) + " " + str(result[i][1]) + "\n")

    f.write("accepted files:\n")
    for e in accept:
        f.write("\t" + e + "\n")

    f.write("rejected files:\n")
    for e in reject:
        f.write("\t" + e + "\n")

    f.write("seed: + str(len(seeds))\n")
    for e in seeds:
        f.write("\t" + e + "\n")

    f.close()

def report_seed(output, id, total, accept, reject, seeds):
    f = open(output, "wt")

    f.write(id + " [seed]\n")
    f.write("line coverage : " + str(total) + "\n")
    f.write("total seed files : " + str(len(seeds)) + "\n")

    f.write("accepted files:\n")
    for e in accept:
        f.write("\t" + e + "\n")

    f.write("rejected files:\n")
    for e in reject:
        f.write("\t" + e + "\n")

    f.close()

def get_gcda_files(src):
    ret = []
    for root, subdirs, files in os.walk(src):
        for file in files:
            if file.endswith(".gcda"):
                ret.append(root + os.sep + file)    
    return ret

def get_file_coverage(src):
    ret = []
    os.system("llvm-cov-6.0 gcov -n -a " + src + " > temp.cov 2> /dev/null")
    f = open("temp.cov")
    lines = f.read().split("\n")
    f.close()
    os.unlink("temp.cov")

    i = 0
    while i < len(lines):
        line = lines[i]
        if not line.startswith("File"):
            i += 1
            continue

        name = line.split()[1]
        if not name.startswith("\'") or not name.endswith("\'"):
            raise RuntimeError("What is this line? : " + line)
        name = name[1:-1]

        i += 1
        line = lines[i]
        percent = line.split(":")[1].split()[0]
        if not percent.endswith("%"):
            raise RuntimeError("What is this line? : " + line)
        percent = float(percent[:-1]) / 100.0

        total = line.split(":")[1].split()[-1]
        total = int(total)

        visited = int(percent * float(total))
        ret.append((name, visited))
        i += 1

    return ret
        
def get_file_coverages(srcs):
    ret = []
    for src in srcs:
        ret = ret + get_file_coverage(src)        
    return ret

def execute_single(executable, run_type, seed, ws):
    cmd = [executable]
    if run_type != "stdin":
        cmd.append(seed)
    output = open(os.devnull, "wt")
    if run_type == "stdin":
        input = open(seed, "rt")
        p = subprocess.Popen(cmd, stdout = output, stderr = output, stdin = input, cwd = ws).communicate()
        input.close()
    else:
        print (cmd)
        p = subprocess.Popen(cmd, stdout = output, stderr = output, cwd = ws).communicate()
    output.close()

def sum_coverage(coverage, workspace):
    accept = set()
    reject = set()
    total = 0

    for e in coverage:
        file = e[0]
        visit = e[1]

        if file.startswith(os.sep + "usr"):
            reject.add(file)
        elif "googletest" in file:
            reject.add(file)
        elif os.sep + "test" + os.sep in file:
            reject.add(file)
        elif file.startswith("tests" + os.sep):
            reject.add(file)
        elif file.endswith("tests.c"):
            reject.add(file)
        elif os.sep + "third_party" + os.sep in file:
            reject.add(file)
        else:
            accept.add(file)
            total += visit

    return accept, reject, total

def process_exec(workspace, id, fuzzer, source, execs, output):
    g_seeds = []
    g_accept = set()
    g_reject = set()
    g_total = 0
    g_ret = []

    gcda_files = get_gcda_files(source)
    for gcda_file in gcda_files:
        os.unlink(gcda_file)

    for e in execs:
        seeds = []

        executable = e[0]
        seed = e[1]

        seeds = []
        origs = []
        for e in os.listdir(seed):
            if os.path.isdir(seed + os.sep + e):
                continue
            if "orig" in e:
                origs.append(seed + os.sep + e)
            else:
                seeds.append(seed + os.sep + e)

        seeds.sort(key=lambda x: os.path.getctime(x))
        init_time = float(os.path.getctime(seeds[0]))
        seeds = origs + seeds

        g_seeds += seeds
    
        for i in range(0, len(seeds)):
            e = seeds[i]

            execute_single(executable, fuzzer, e, source)

            gcda_files = get_gcda_files(source)
            file_coverages = get_file_coverages(gcda_files)
            accept, reject, total = sum_coverage(file_coverages, source)
            if "orig" in os.path.basename(e):
                time = 0
            else:
                time = float(os.path.getctime(e)) - init_time

            print ("[" + str(i) + "/" + str(len(seeds))+ "] " + str(time) + " " + str(total) + "(" + os.path.basename(e) + ")")

            g_accept = g_accept | accept
            g_reject = g_reject | reject

            if total > g_total:
                print ("ACCEPTED\n")
                g_total = total
                g_ret.append((time, g_total))

    report_exec(output, id, g_ret, g_accept, g_reject, g_seeds)

def process_seed(workspace, id, fuzzer, source, execs, output):
    g_seeds = []

    gcda_files = get_gcda_files(source)
    for gcda_file in gcda_files:
        os.unlink(gcda_file)

    for e in execs:
        seeds = []

        executable = e[0]
        seed = e[1]

        for e in os.listdir(seed):
            if os.path.isdir(seed + os.sep + e):
                continue
            seeds.append(seed + os.sep + e)

        for i in range(0, len(seeds)):
            e = seeds[i]
            execute_single(executable, fuzzer, e, source)
            print (str(i) + "/" + str(len(seeds)) + " for " + os.path.basename(executable))

        g_seeds += seeds

    gcda_files = get_gcda_files(source)
    file_coverages = get_file_coverages(gcda_files)
    accept, reject, total = sum_coverage(file_coverages, source)
    report_seed(output, id, total, accept, reject, g_seeds)

def main():
    workspace, id, fuzzer, source, type, output, execs = get_option(sys.argv)
    source = workspace + os.sep + source
    for i in range(0, len(execs)):
        execs[i] = ( workspace + os.sep + execs[i][0], workspace + os.sep + execs[i][1])

    if type == "exec":
        process_exec(workspace, id, fuzzer, source, execs, output)
    elif type == "seed":
        process_seed(workspace, id, fuzzer, source, execs, output)
    else:
        raise RuntimeError("unknown type(" + type + ")")

if __name__ == "__main__":
    main()
