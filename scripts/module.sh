#!/usr/bin/env bash
### configure kernel module

module_path="/sys/devices/virtual/memory_address_tracer/memory_address_tracer"
device_path="/dev/memory_address_tracer"

function usage() {
    cat << EOF
Usage: $script_name [-h] [-p] [COMMAND]

Configure Linux kernel module.

COMMAND:
    set                          Configure kernel module for profiling.
    reset                        Reset configuration of kernel module.
    showconfig                   Show configuration of kernel module.
    showdebug                    Show debug information of kernel module.
    showsamples                  Show range counters.
    showsamplesall               Show range counters of all CPUs.
    showbuffers                  Show buffer statistics.
    showbuffersall               Show buffer statistics of all CPUs.
    write                        Write all per-core buffers to disk.

Options:
    -h, --help                    Show help message and exit.
    -p, --physical-address        Set up profiling of physical address
                                  instead of virtual address.
    -s, --buffer-size <size>      Set size of per-core address buffers.
EOF
}

### parse command line arguments
phys_addr=
cmd="showconfig"
buffer_bytes="$((2**30))" ### 1GiB buffer per core

while [ "$#" -gt 0 ]; do
    case "$1" in
    -h|--help)
        usage
        exit 0
        ;;
    -p|--physical-address)
        phys_addr="true"
        shift
        ;;
    -s|--buffer-size)
        ### parse size with suffix to bytes
        buffer_bytes="$(numfmt --from=auto $2)"
        shift
        shift
        ;;
    set|reset|showconfig|showdebug|showsamples|showsamplesall|showbuffers|showbuffersall|write)
        cmd="$1"
        shift
        break
        ;;
    *)
        echo "$script_name: Unknown parameter $1"
        usage
        exit 1
        ;;
    esac
done
### divide by byte size of an address and by number of buffers we allocate per core
buffer_size="$(($buffer_bytes / 8 / 2))"

if [[ "$EUID" -ne 0 ]]; then
    echo "error: no root rights"
    echo "run as root"
    exit 1
fi

### check that kernel module is loaded and IO files are available
mod_files="get_addr perf_force_lpebs perf_no_throttling samples samples_total"
mod_files+=" samples_total_nz buffers buffers_enabled cpu buffers_bytes phys_addr"
for f in $mod_files
do
    if [[ ! -f $module_path/$f ]]; then
        echo "error: $module_path/$f not found"
        echo "is kernel module loaded?"
        exit 1
    fi
done
if [[ ! -e $device_path ]]; then
    echo "error: $device_path not found"
    echo "is kernel module loaded?"
    exit 1
fi

if [[ "$cmd" == "reset" ]]; then
    echo 100000 > /proc/sys/kernel/perf_event_max_sample_rate
    echo 25 > /proc/sys/kernel/perf_cpu_time_max_percent
    for f in buffers_enabled perf_no_throttling perf_force_lpebs module_debug kernel_debug phys_addr samples buffers get_addr
    do
        [[ -f $module_path/$f ]] && echo 0 > $module_path/$f
    done
    [[ -f $module_path/cpu ]] && echo -1 > $module_path/cpu
elif [[ "$cmd" == "set" ]]; then
    echo 100000 > /proc/sys/kernel/perf_event_max_sample_rate
    echo 99 > /proc/sys/kernel/perf_cpu_time_max_percent
    echo 1 > $module_path/buffers_enabled
    echo 1 > $module_path/perf_no_throttling
    echo 1 > $module_path/perf_force_lpebs
    echo 0 > $module_path/module_debug
    echo 0 > $module_path/kernel_debug
    if [[ -z "$phys_addr" ]]; then
        echo 0 > $module_path/phys_addr
    else
        echo 1 > $module_path/phys_addr
    fi
    echo 0 > $module_path/samples
    echo "allocate per-core buffer with $buffer_bytes bytes ($(numfmt --to=si $buffer_bytes))"
    echo "$buffer_size" > $module_path/buffers
    echo -1 > $module_path/cpu
    echo 1 > $module_path/get_addr
elif [[ "$cmd" == "showconfig" ]]; then
    for f in perf_event_max_sample_rate perf_cpu_time_max_percent
    do
        file="/proc/sys/kernel/$f"
        printf "%-26s: %s\n" "$(basename $file)" "$(cat $file)"
    done
    for f in buffers_enabled cpu get_addr kernel_debug perf_no_throttling perf_force_lpebs phys_addr samples_total samples_total_nz
    do
        file="$module_path/$f"
        [[ -f "$file" ]] && printf "%-26s: %s\n" "$(basename $file)" "$(cat $file)"
    done
elif [[ "$cmd" == "showdebug" ]]; then
    file="$module_path/module_debug"
    [[ -f "$file" ]] && cat $file
elif [[ "$cmd" == "showsamples" ]]; then
    file="$module_path/samples"
    [[ -f "$file" ]] && cat $file
elif [[ "$cmd" == "showsamplesall" ]]; then
    old="$(cat $module_path/cpu)"
    file="$module_path/samples"
    ### show header
    cat $file | head -n 4
    ### show range counter for each CPU
    for cpu in $(seq -s ' ' 0 1 $(($(nproc)-1)))
    do
        echo $cpu > $module_path/cpu
        tmp="$(cat $file)"
        if [[ $(echo -e "$tmp" | wc -l ) -gt 2 ]]; then
            echo -e "$tmp" | tail -n 1
        fi
    done
    echo $old > $module_path/cpu
elif [[ "$cmd" == "showbuffers" ]]; then
    file="$module_path/buffers"
    [[ -f "$file" ]] && cat $file
elif [[ "$cmd" == "showbuffersall" ]]; then
    old="$(cat $module_path/cpu)"
    for cpu in $(seq 0 1 $(($(nproc)-1)))
    do
        [[ -f $module_path/buffers ]] && echo $cpu > $module_path/cpu && cat $module_path/buffers | grep CPU
    done
    echo $old > $module_path/cpu
elif [[ "$cmd" == "write" ]]; then
    old="$(cat $module_path/cpu)"
    for cpu in $(seq 0 1 $(($(nproc)-1)))
    do
        if [[ -f $module_path/buffers ]]; then
            echo $cpu > $module_path/cpu
            available="$(cat $module_path/buffers_bytes)"
            if [[ "$available" -gt 0 ]]; then
                ofile="$(printf "CPU%03d.bin" "$cpu")"
                echo "writing $ofile ..."
                head --bytes=$available $device_path > $ofile
            fi
        fi
    done
    echo $old > $module_path/cpu
else
    echo "unknow command: $cmd"
    exit 1
fi

