import subprocess
import re
from datetime import datetime
from matplotlib import pylab
import matplotlib.pyplot as plt
from matplotlib.pyplot import *
from matplotlib.pylab import *
import math

SETTINGS_FILE_LOCATION = "xml/settings.xml"


def print_timestamp(string):
    print("[" + str(datetime.now().strftime("%Y-%m-%d %H:%M:%S")) + "] " + str(string))

class GenerationStatistics:
    """
    Stores generation statistics values.
    """
    def __init__(self):
        self.number = None
        self.time = None
        self.min = None
        self.max = None
        self.mean = None
        self.median = None
        self.lowerq = None
        self.upperq = None


class MinAvgMaxTriplet:
    def __init__(self):
        self.min = None
        self.avg = None
        self.max = None


class ProcessStatistics:
    def __init__(self):
        self.pid = None
        self.total_time = None
        self.initial_generation = None
        self.prerequisite_initialization = None
        self.complete_generation = MinAvgMaxTriplet()
        self.fitness_computation = MinAvgMaxTriplet()
        self.population_fitness_sending = MinAvgMaxTriplet()
        self.selection = MinAvgMaxTriplet()
        self.survivor_indices_broadcast = MinAvgMaxTriplet()
        self.survivor_processing = MinAvgMaxTriplet()
        self.survivor_allgather = MinAvgMaxTriplet()
        self.repopulation = MinAvgMaxTriplet()
        self.population_adjustment = MinAvgMaxTriplet()


class Run:
    def __init__(self):
        self.population_size = None
        self.survivor_ratio = None
        self.rounds = None
        self.mutation_probability = None

        self.settings_generated = False
        self.run_ran = False
        self.output_processed = False
        self.run_output = ""

        self.generations = []
        self.process_statistics = []

    def generate_settings(self):
        """
        Generates the XML settings file.
        """
        if self.population_size is None or self.survivor_ratio is None or self.rounds is None or self.mutation_probability is None:
            raise Exception("Run settings not set. ")

        with open(SETTINGS_FILE_LOCATION, "r") as f:
            contents = f.read()

        contents = re.sub(r"<population_size>\d+</population_size>", "<population_size>{}</population_size>".format(str(self.population_size)), contents)
        contents = re.sub(r"<survivor_ratio>\d+\.\d+</survivor_ratio>", "<survivor_ratio>{}</survivor_ratio>".format(str(self.survivor_ratio)), contents)
        contents = re.sub(r"<rounds>\d+</rounds>", "<rounds>{}</rounds>".format(str(self.rounds)), contents)
        contents = re.sub(r"<mutation_probability>\d+\.\d+</mutation_probability>", "<mutation_probability>{}</mutation_probability>".format(str(self.mutation_probability)), contents)

        with open(SETTINGS_FILE_LOCATION, "w") as f:
            f.write(contents)

        self.settings_generated = True

    def run(self):
        """
        Runs the program and captures the output.
        """
        if not self.settings_generated:
            raise Exception("Settings have not yet been generated. ")

        try:
            output = subprocess.check_output(["./launch.sh", "run", "main_launch"], cwd="out/", stderr=subprocess.DEVNULL)
            self.run_output = output.decode("utf-8")
        except subprocess.CalledProcessError as e:
            print_timestamp("Error in the called process. Ignoring because it's expected. ")
            self.run_output = e.output.decode("utf-8")

        self.run_ran = True

    def process_output(self, output_text=None):
        """
        Processes the program output.
        """
        if not self.run_ran and output_text is None:
            raise Exception("Run has not yet been ran. ")

        if output_text is not None:
            self.run_ran = True
            self.run_output = output_text

        generation_statistics_strings = re.findall(r"GENERATION \d+.*?upper quartile: -?\d+\.\d+", self.run_output, re.DOTALL)
        for string in generation_statistics_strings:
            match = re.match(r"GENERATION (?P<gen>\d+) \((?P<gentime>\d+\.\d+) s\).*?"
                             r"min:\s+(?P<min>-?\d+\.\d+).*?"
                             r"max:\s+(?P<max>-?\d+\.\d+).*?"
                             r"mean:\s+(?P<mean>-?\d+\.\d+).*?"
                             r"median:\s+(?P<median>-?\d+\.\d+).*?"
                             r"lower quartile:\s+(?P<lowerq>-?\d+\.\d+).*?"
                             r"upper quartile:\s+(?P<upperq>-?\d+\.\d+)",
                             string, re.DOTALL)
            if match is None:
                print_timestamp(string)
                raise Exception("Generation does not match format. ")

            g = match.groupdict()
            gen = GenerationStatistics()
            gen.number = int(g["gen"])
            gen.time = float(g["gentime"])
            gen.min = float(g["min"])
            gen.max = float(g["max"])
            gen.mean = float(g["mean"])
            gen.median = float(g["median"])
            gen.lowerq = float(g["lowerq"])
            gen.upperq = float(g["upperq"])
            self.generations.append(gen)

        self.generations.sort(key=lambda g: g.number)

        process_statistics_strings = re.findall(r"Process \d+: Genetic.*?stats end.", self.run_output, re.DOTALL)
        for string in process_statistics_strings:
            match = re.match(r"Process (?P<processid>\d+).*?"
                             r"Total time taken:\s+(?P<totaltime>.*?) s.*?"
                             r"Initial generation:\s+(?P<initialgen>.*?) s.*?"
                             r"Prerequisite initialization:\s+(?P<prereqinit>.*?) s.*?"
                             r"Complete generation:\s+min (?P<compgenmin>.*?) s; avg (?P<compgenavg>.*?) s; max (?P<compgenmax>.*?) s.*?"
                             r"Fitness computation:\s+min (?P<fitcompmin>.*?) s; avg (?P<fitcompavg>.*?) s; max (?P<fitcompmax>.*?) s.*?"
                             r"Population fitness sending:\s+min (?P<popfitsendmin>.*?) s; avg (?P<popfitsendavg>.*?) s; max (?P<popfitsendmax>.*?) s.*?"
                             r"Selection:\s+min (?P<selectionmin>.*?) s; avg (?P<selectionavg>.*?) s; max (?P<selectionmax>.*?) s.*?"
                             r"Survivor indices broadcast:\s+min (?P<surindbroadmin>.*?) s; avg (?P<surindbroadavg>.*?) s; max (?P<surindbroadmax>.*?) s.*?"
                             r"Survivor processing:\s+min (?P<surprocmin>.*?) s; avg (?P<surprocavg>.*?) s; max (?P<surprocmax>.*?) s.*?"
                             r"Survivor allgather:\s+min (?P<surallgathermin>.*?) s; avg (?P<surallgatheravg>.*?) s; max (?P<surallgathermax>.*?) s.*?"
                             r"Repopulation:\s+min (?P<repopmin>.*?) s; avg (?P<repopavg>.*?) s; max (?P<repopmax>.*?) s.*?"
                             r"Population adjustment:\s+min (?P<popadjmin>.*?) s; avg (?P<popadjavg>.*?) s; max (?P<popadjmax>.*?) s.*?"
                             r"Process \d+ stats end\.",
                             string, re.DOTALL)
            if match is None:
                print_timestamp(string)
                raise Exception("Process statistics do not match format. ")

            g = match.groupdict()
            stats = ProcessStatistics()
            stats.pid = int(g["processid"])
            stats.total_time = float(g["totaltime"])
            stats.initial_generation = float(g["initialgen"])
            stats.prerequisite_initialization = float(g["prereqinit"])

            stats.complete_generation.min = float(g["compgenmin"])
            stats.complete_generation.avg = float(g["compgenavg"])
            stats.complete_generation.max = float(g["compgenmax"])

            stats.fitness_computation.min = float(g["fitcompmin"])
            stats.fitness_computation.avg = float(g["fitcompavg"])
            stats.fitness_computation.max = float(g["fitcompmax"])

            stats.population_fitness_sending.min = float(g["popfitsendmin"])
            stats.population_fitness_sending.avg = float(g["popfitsendavg"])
            stats.population_fitness_sending.max = float(g["popfitsendmax"])

            stats.selection.min = float(g["selectionmin"])
            stats.selection.avg = float(g["selectionavg"])
            stats.selection.max = float(g["selectionmax"])

            stats.survivor_indices_broadcast.min = float(g["surindbroadmin"])
            stats.survivor_indices_broadcast.avg = float(g["surindbroadavg"])
            stats.survivor_indices_broadcast.max = float(g["surindbroadmax"])

            stats.survivor_processing.min = float(g["surprocmin"])
            stats.survivor_processing.avg = float(g["surprocavg"])
            stats.survivor_processing.max = float(g["surprocmax"])

            stats.survivor_allgather.min = float(g["surallgathermin"])
            stats.survivor_allgather.avg = float(g["surallgatheravg"])
            stats.survivor_allgather.max = float(g["surallgathermax"])

            stats.repopulation.min = float(g["repopmin"])
            stats.repopulation.avg = float(g["repopavg"])
            stats.repopulation.max = float(g["repopmax"])

            stats.population_adjustment.min = float(g["popadjmin"])
            stats.population_adjustment.avg = float(g["popadjavg"])
            stats.population_adjustment.max = float(g["popadjmax"])

            self.process_statistics.append(stats)

        self.process_statistics.sort(key=lambda s: s.pid)
        self.output_processed = True


def import_memory_data(filename):
    """
    Imports the special memory data format.
    # are comments, empty lines ignored, pairs of integer
    """
    with open(filename, "r") as f:
        contents = f.read()

    lines = contents.splitlines()
    avals = []
    bvals = []
    for line in lines:
        line = line.strip()
        if line == "" or line.startswith("#"):
            continue

        spl = line.split(" ")
        assert len(spl) == 2
        avals.append(int(spl[0].strip()))
        bvals.append(int(spl[1].strip()))
    assert len(avals) == len(bvals)
    return avals, bvals


def multi_run_round_times_box(input_filenames, processor_counts):
    """
    Generates a plot with round run times (also whiskers) for multiple runs.
    The input filenames and processor counts must be the same length.
    The processor count at position i matches the file at position i.
    """
    assert len(input_filenames) == len(processor_counts)
    files = [open(input_filename, "r") for input_filename in input_filenames]
    contents_strings = [f.read() for f in files]
    for f in files:
        f.close()

    means = []
    valueses = []
    for count, contents in zip(processor_counts, contents_strings):
        run = Run()
        run.process_output(contents)
        values = [g.time for g in run.generations][1:]  # skip the first, as it's initial generation
        means.append(sum(values) / len(values))
        valueses.append(values)

    # whis means whiksers at 1st and 99th percentile
    # sym="" means don't plot points
    boxplot(valueses, sym="", whis=[1, 99], labels=processor_counts)
    # also plot a line for means
    plt.grid(True)
    # simulate points because we have non-continuous data labels
    plt.plot(range(1, len(processor_counts) + 1), means, color="0.4")
    plt.xlabel("procesi")
    plt.ylabel("čas [s]", rotation="vertical")
    plt.ylim(ymin=0)
    pylab.show()


def memory_plot(filename, format, xlabel, ylabel, normalize=False):
    """
    Plots memory, offers normalization. Markers and a semi-transparent dashed line.
    """
    xs, ys = import_memory_data(filename)

    if normalize:
        ys = [x * y for x, y in zip(xs, ys)]

    plt.grid(True)
    plt.ylim(ymin=0)
    plt.plot(xs, ys, format)
    plt.plot(xs, ys, color="0.7", linestyle="dashed", linewidth=1)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.show()


def round_times_plot(filename):
    """
    Plots a scatter plot of generation times.
    """
    with open(filename, "r") as f:
        contents = f.read()
    run = Run()
    run.process_output(contents)

    times = [g.time for g in run.generations][1:]

    plt.plot(times, marker="x", linestyle="", color="0.1")
    plt.xlabel("generacija")
    plt.ylabel("čas [s]")
    plt.show()


def round_fitnesses_plot(filename, pattern, minround=None, maxround=None, ylimitmin=None):
    """
    Prints a scatter plot with a specific pattern, offers min/max functionality.
    """
    with open(filename, "r") as f:
        contents = f.read()
    run = Run()
    run.process_output(contents)

    fitnesses = [g.max for g in run.generations][1:]

    if minround is not None:
        fitnesses = fitnesses[minround:]

    if maxround is not None:
        fitnesses = fitnesses[:(maxround + 1 - (minround or 0))]

    if ylimitmin is not None:
        plt.ylim(ymin=ylimitmin)

    plt.ticklabel_format(style="sci", axis="y", scilimits=(0, 0))
    plt.plot(range(500, 999), fitnesses, pattern, linestyle="None")
    plt.xlabel("generacija")
    plt.ylabel("ustreznost")


def multi_run_speedup_efficiency(input_filenames, processor_counts):
    """
    Creates a double-bar plot with speedups and efficiencies.
    Input filenames and processor counts must have the same length, elements correspond.
    """
    assert len(input_filenames) == len(processor_counts)
    files = [open(input_filename, "r") for input_filename in input_filenames]
    contents_strings = [f.read() for f in files]
    for f in files:
        f.close()

    means = []
    for count, contents in zip(processor_counts, contents_strings):
        run = Run()
        run.process_output(contents)
        values = [g.time for g in run.generations][1:]  # skip the first, as it's initial generation
        means.append(sum(values) / len(values))

    sequential_time = means[0]
    speedups = [sequential_time / m for m in means]
    efficiencies = [s / p for s, p in zip(speedups, processor_counts)]

    # also print for tabular
    print("  p |  S(n,p)  |  E(n,p)")
    print("------------------------")
    for p, s, e in zip(processor_counts, speedups, efficiencies):
        print(" {:2d} |   {:.3f}  |  {:.3f}".format(p, s, e))

    plt.grid(True)
    width = 0.4
    plt.bar([i - width for i in range(1, len(processor_counts) + 1)], speedups, align="edge", width=width, color=(11 / 255, 157 / 255, 222 / 255))
    plt.bar(range(1, len(processor_counts) + 1), efficiencies, align="edge", width=width, color=(217 / 255, 120 / 255, 0 / 255))
    plt.xticks(range(1, 14), processor_counts)
    plt.xlabel("procesi")
    plt.ylabel("pohitritev/učinkovitost")
    plt.legend(["Pohitritev", "Učinkovitost"], loc=2)
    plt.show()


def time_ratios_computation(filename):
    """
    Computes time ratios.
    """
    with open(filename, "r") as f:
        contents = f.read()
    run = Run()
    run.process_output(contents)

    sequential_time = 0
    parallel_time = 0
    communication_time = 0

    nprocs = len(run.process_statistics)
    num_gens = len(run.generations)
    for stat in run.process_statistics:
        sequential_time += stat.selection.avg * num_gens
        parallel_time += (stat.fitness_computation.avg + stat.survivor_processing.avg + stat.repopulation.avg) * num_gens
        communication_time += (stat.population_fitness_sending.avg + stat.survivor_indices_broadcast.avg + stat.survivor_allgather.avg + stat.population_adjustment.avg) * num_gens

    # sequential_time /= nprocs
    parallel_time /= nprocs
    communication_time /= nprocs
    total = sequential_time + parallel_time + communication_time

    print("sequential: {} ({})".format(sequential_time, sequential_time / total))
    print("parallel: {} ({})".format(parallel_time, parallel_time / total))
    print("communication: {} ({})".format(communication_time, communication_time / total))
    print("total: {}".format(total))

def analyze_single(filename):
    with open(filename, "r") as f:
        contents = f.read()
    run = Run()
    run.process_output(contents)

    avg_round_time = sum([g.time for g in run.generations]) / len(run.generations)

    print("Average round time: {}".format(avg_round_time))

if __name__ == "__main__":
    figure()

    # cores = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 16, 32, 64]
    # filenames = ["out/saved/ubuntuvm-{}-5000-OUTPUT".format(core) for core in cores]
    # multi_run_round_times_box(filenames, cores)
    # multi_run_speedup_efficiency(filenames, cores)

    # analyze_single("out/saved/three-all-2000-eth-REAL-1000RDS-OUTPUT")

    # memory_plot("out/saved/ubuntuvm-1_64-MEMORY", "bo", "procesi", "pomnilnik [MB]")
    # memory_plot("out/saved/ubuntuvm-1_64-MEMORY", "bo", "procesi", "pomnilnik [MB]", normalize=True)
    # memory_plot("out/saved/ubuntuvm-8-1000_10000-MEMORY", "bo", "velikost populacije", "pomnilnik [MB]")

    # round_times_plot("out/saved/three-all-5000-wifi-OUTPUT")
    # round_times_plot("out/saved/three-all-5000-eth-OUTPUT")
    # round_times_plot("out/saved/three-all-2000-eth-REAL-1000RDS-OUTPUT")
    round_times_plot("out/saved/ubuntuvm-8-5000-OUTPUT")

    # round_fitnesses_plot("out/saved/ubuntuvm-2-200-OUTPUT", "r+", maxround=200)
    # round_fitnesses_plot("out/saved/ubuntuvm-8-1000-OUTPUT", "g.", maxround=200)
    # round_fitnesses_plot("out/saved/ubuntuvm-8-10000-OUTPUT", "bx", maxround=200)
    # round_fitnesses_plot("out/saved/three-all-2000-eth-REAL-1000RDS-OUTPUT", "bx")
    # round_fitnesses_plot("out/saved/three-all-2000-eth-REAL-1000RDS-OUTPUT", "bx", minround=500)
    # plt.legend(["N = 200", "N = 1000", "N = 10000"], loc=4)
    # plt.show()

    # time_ratios_computation("out/saved/ubuntuvm-64-5000-OUTPUT")
    # time_ratios_computation("out/saved/three-all-2000-eth-REAL-1000RDS-OUTPUT")
