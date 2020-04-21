#!/bin/bash
# DPU_PROFILING_RESULT_PARSER contains the result parser to use
#   - dpu_profiling_print_summary
#   - dpu_profiling_plot
# $@ = Executable and its arguments

# Retrieve current library path from dpu-pkg-config
pkg_config_lib=$(dpu-pkg-config --variable=libdir dpu)
# Extract the library path from pkg-config output
library_path=${pkg_config_lib}
parser_path="$library_path/../bin"
parser_result=${DPU_PROFILING_RESULT_PARSER:-dpu_profiling_print_summary}

list_probes=(   "libdpu;dpu_copy_to_wram_for_dpu=dpu_copy_to_wram_for_dpu dpu->rank wram_word_offset nb_of_words"
                "libdpu;dpu_copy_to_wram_for_dpu_exit=dpu_copy_to_wram_for_dpu%return"
                "libdpu;dpu_copy_from_wram_for_dpu=dpu_copy_from_wram_for_dpu dpu->rank wram_word_offset nb_of_words"
                "libdpu;dpu_copy_from_wram_for_dpu_exit=dpu_copy_from_wram_for_dpu%return"
                "libdpu;dpu_copy_to_wram_for_rank=dpu_copy_to_wram_for_rank rank wram_word_offset nb_of_words"
                "libdpu;dpu_copy_to_wram_for_rank_exit=dpu_copy_to_wram_for_rank%return"
                "libdpu;dpu_copy_to_iram_for_dpu=dpu_copy_to_iram_for_dpu dpu->rank iram_instruction_index nb_of_instructions"
                "libdpu;dpu_copy_to_iram_for_dpu_exit=dpu_copy_to_iram_for_dpu%return"
                "libdpu;dpu_copy_to_iram_for_rank=dpu_copy_to_iram_for_rank rank iram_instruction_index nb_of_instructions"
                "libdpu;dpu_copy_to_iram_for_rank_exit=dpu_copy_to_iram_for_rank%return"
                "libdpu;dpu_copy_from_iram_for_dpu=dpu_copy_from_iram_for_dpu dpu->rank iram_instruction_index nb_of_instructions"
                "libdpu;dpu_copy_from_iram_for_dpu_exit=dpu_copy_from_iram_for_dpu%return"
                "libdpu;dpu_transfer_matrix_add_dpu=dpu_transfer_matrix_add_dpu dpu transfer_matrix size offset_in_mram"
                "libdpu;dpu_transfer_matrix_clear_dpu=dpu_transfer_matrix_clear_dpu dpu transfer_matrix"
                "libdpu;dpu_transfer_matrix_clear_all=dpu_transfer_matrix_clear_all transfer_matrix"
                "libdpu;dpu_transfer_matrix_free=dpu_transfer_matrix_free transfer_matrix"
                "libdpu;dpu_copy_to_mrams=dpu_copy_to_mrams rank transfer_matrix"
                "libdpu;dpu_copy_to_mrams_exit=dpu_copy_to_mrams%return"
                "libdpu;dpu_copy_from_mrams=dpu_copy_from_mrams rank transfer_matrix"
                "libdpu;dpu_copy_from_mrams_exit=dpu_copy_from_mrams%return"
                "libdpuhw;log_rank_path=log_rank_path rank path:string")

record_string=""
for item in "${list_probes[@]}"; do
        # Redirect all outputs to /dev/null since if the addition of a probe
        # fails, perf record will fail.
        library_name=$(echo $item | awk -F ";" '{print $1}')
        probe=$(echo $item | awk -F ";" '{print $2}')
        perf probe --exec="$library_path/$library_name.so" --add "$probe" &> /dev/null
        probe_name=$(echo $probe | awk -F "=" '{print $1}')
        if [[ "$probe_name" = *exit* ]]; then
                probe_name="$probe_name""__return"
        fi
        record_string="$record_string -e probe_$library_name:$probe_name"
done

perf record $record_string -o perf.data -aqR "$@"

for item in "${list_probes[@]}"; do
        probe=$(echo $item | awk -F ";" '{print $2}')
        probe_name=$(echo $probe | awk -F "=" '{print $1}')
        if [[ "$probe_name" = *exit* ]]; then
                probe_name="$probe_name""__return"
        fi
        perf probe -d "$probe_name" &> /dev/null
done

perf script -i perf.data -s $parser_path/dpu_profiling_memory_transfer_traces_processing.py -- --result-parser $parser_result
