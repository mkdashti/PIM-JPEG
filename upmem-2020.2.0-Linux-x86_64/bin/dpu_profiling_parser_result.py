class PrintResult:
        RED = '\033[31m'
        GREEN = '\033[92m'
        ENDC = '\033[0m'
        BOLD = '\033[1m'

        @staticmethod
        def format_size_unit(size, iram_size):
                if (iram_size):
                    size_type = "insn"
                else:
                    size_type = "B"

                if size < 1024:
                        return size_type
                elif size < 1024 * 1024:
                        return "K{}".format(size_type)
                elif size < 1024 * 1024 * 1024:
                        return "M{}".format(size_type)

                return "G{}".format(size_type)

        @staticmethod
        def format_size(size, iram_size):
                if (iram_size):
                    size_type = "insn"
                else:
                    size_type = "B"

                if size < 1024:
                        print_size = "{:8.3f}{}".format(size, size_type)
                elif size < 1024 * 1024:
                        print_size = "{:8.3f}K{}".format(size / 1024, size_type)
                elif size < 1024 * 1024 * 1024:
                        print_size = "{:8.3f}M{}".format(size / 1024 / 1024, size_type)
                else:
                        print_size = "{:8.3f}G{}".format(size / 1024 / 1024 / 1024, size_type)

                return print_size


class Duration:
    @staticmethod
    def nanoseconds(secs, nsecs):
        return secs * 1000000000 + nsecs

    @staticmethod
    def seconds(nsecs):
        return nsecs / float(1000000000)
