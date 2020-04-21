from dpu_profiling_parser_result import PrintResult, Duration

def parse_init():
    pass

def parse_end():
    pass

def print_transfer_info(size, total_duration, nb_size, iram_bandwidth = False):
    average = Duration.seconds(total_duration) / nb_size
    print_size = PrintResult.format_size(size, iram_bandwidth)
    print_bandwidth = PrintResult.format_size(size / average, iram_bandwidth)

    print("\tsize = {}\tnb = {:6}\tbandwidth = {}/s".format(print_size, nb_size, print_bandwidth))

def print_all_transfer_info(xfer_info, nb_per_size_xfer, rank_path, iram_bandwidth = False):
    print(PrintResult.GREEN + PrintResult.BOLD + "{}:".format(rank_path) + PrintResult.ENDC)
    for size, duration in xfer_info.items():
        print_transfer_info(size, duration, nb_per_size_xfer[rank_path][size], iram_bandwidth)

def print_summary_infos(infos, type_name, iram_bandwidth = False):
        if (len(infos) == 0):
            return

        print(PrintResult.RED + PrintResult.BOLD + "*** {} ***".format(type_name) + PrintResult.ENDC)

        per_size_xfer = {}
        nb_per_size_xfer = {}

        per_size_xfer["Average"] = {}
        nb_per_size_xfer["Average"] = {}

        # size, duration, rank_path
        for xfer_info in infos:
                start_ns = xfer_info.start_ns
                end_ns = xfer_info.end_ns
                size = xfer_info.size
                rank_path = xfer_info.rank_path

                duration = end_ns - start_ns

                if rank_path not in per_size_xfer:
                        per_size_xfer[rank_path] = {}
                        nb_per_size_xfer[rank_path] = {}

                # Per-rank stats
                per_size_xfer[rank_path][size] = per_size_xfer[rank_path].get(size, 0) + duration
                nb_per_size_xfer[rank_path][size] = nb_per_size_xfer[rank_path].get(size, 0) + 1
                # Average stats
                per_size_xfer["Average"][size] = per_size_xfer["Average"].get(size, 0) + duration
                nb_per_size_xfer["Average"][size] = nb_per_size_xfer["Average"].get(size, 0) + 1

        for rank_path, xfer_info in sorted(per_size_xfer.items()):
                print_all_transfer_info(xfer_info, nb_per_size_xfer, rank_path)

def parse_wram_write(infos):
        print_summary_infos(infos, "WRAM write")

def parse_wram_read(infos):
        print_summary_infos(infos, "WRAM read")

def parse_mram_read(infos):
        print_summary_infos(infos, "MRAM read")

def parse_mram_write(infos):
        print_summary_infos(infos, "MRAM write")

def parse_iram_read(infos):
        print_summary_infos(infos, "IRAM read", True)

def parse_iram_write(infos):
        print_summary_infos(infos, "IRAM write", True)

