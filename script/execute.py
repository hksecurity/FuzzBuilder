import sys, os, subprocess

def log(prefix, msg):
    print ("[" + prefix + "] " + msg)

def usage():
    log("I", "python " + os.path.basename(__file__) + " ${type}[stdin|file] ${path for executable} ${path for seeds}")

def option():
    if len(sys.argv) < 4:
        usage()
        sys.exit(0)

    run_type = sys.argv[1]
    if run_type != "stdin" and run_type != "file":
        usage()
        sys.exit(0)

    executable = sys.argv[2]
    seed = sys.argv[3]

    ret = {}
    ret["type"] = run_type
    ret["exec"] = executable
    ret["seed"] = seed
    return ret

def execute_single(executable, run_type, seed):
    cmd = [executable]
    if run_type != "stdin":
        cmd.append(seed)
    output = open(os.devnull, "wt")
    if run_type == "stdin":
        input = open(seed, "rt")
        p = subprocess.Popen(cmd, stdout = output, stderr = output, stdin = input).communicate()
        input.close()
    else:
        p = subprocess.Popen(cmd, stdout = output, stderr = output).communicate()
    output.close()

def execute_batch(src):
    seeds = []

    try:
        for e in os.listdir(src["seed"]):
            if os.path.isdir(src["seed"] + os.sep + e):
                continue
            seeds.append(src["seed"] + os.sep + e)
    except IOError:
        log("E", "Directory not found(" + src[seed] + ")")

    for i in range(0, len(seeds)):
        log("I", "Executing " + str(i+1) + "/" + str(len(seeds)))
        execute_single(src["exec"], src["type"], seeds[i])

def main():
    cmd_option = option()
    execute_batch(cmd_option)

if __name__ == "__main__":
    main()
