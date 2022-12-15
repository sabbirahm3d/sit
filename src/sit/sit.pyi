from typing import Callable, Literal, final

class SIT:
    def __init__(
        self,
        ipc: Literal["sock", "zmq"],
        module: str,
        lib: str,
        width_macros: dict[str, int] | None = ...,
        module_dir: str = ...,
        lib_dir: str = ...,
        desc: str = ...,
        driver_template_path: str = ...,
        component_template_path: str = ...,
    ) -> None: ...
    def _get_driver_inputs(self) -> str: ...
    def _get_driver_outputs(self) -> str: ...
    def _get_driver_defs(self) -> str: ...
    @staticmethod
    def _sig_fmt(
        fmt: str, split_func: Callable[[str], str], array: list[str], delim: str
    ) -> str: ...
    def _get_signal_width_from_macro(self, signal_type: str) -> str: ...
    def _get_all_ports(self) -> None: ...
    @final
    def set_ports(self, ports: tuple[tuple[str, str, str], ...]) -> None: ...
    def __get_comp_defs(self) -> dict[str, str]: ...
    def __generate_comp_str(self) -> str: ...
    def __generate_driver_str(self) -> str: ...
    @final
    def generate_bbox(self) -> None: ...
    @final
    def fixed_width_float_output(self, precision: int) -> None: ...
    @final
    def disable_runtime_warnings(self, warnings: str | list[str]) -> None: ...
    @staticmethod
    def _get_ints(signal: str) -> int: ...
    @staticmethod
    def _get_num_digits(signal: int) -> int: ...
