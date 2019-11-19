#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os


class BoilerPlate(object):

    def __init__(self, module, lib, ipc, drvr_templ_path, comp_templ_path,
                 desc="", module_dir="", lib_dir=""):
        """Constructor for BoilerPlate.

        Arguments:
            module {str} -- module name
            lib {str} -- SST library name
            ipc {str} -- type of IPC. Supported options are ("sock", "zmq")
            drvr_templ_path {str} -- path to the black box-driver boilerplate
            comp_templ_path {str} -- path to the black box-model boilerplate
            desc {str} -- description of the SST model (default: {""})
        """
        if ipc in ("sock", "zmq"):
            self.ipc = ipc
        else:
            raise ValueError("Incorrect IPC protocol selected")

        self.module_dir = module_dir
        self.lib_dir = lib_dir
        self.module = module
        self.lib = lib
        self.drvr_templ_path = drvr_templ_path
        self.comp_templ_path = comp_templ_path
        self.desc = desc

        self.clocks = []
        self.inputs = []
        self.outputs = []
        self.inouts = []
        self.ports = []
        self.buf_size = 0

        if self.ipc == "sock":

            # component attributes
            self.sig_type = """SocketSignal"""

        elif self.ipc == "zmq":

            # component attributes
            self.sig_type = """ZMQSignal"""

        # shared attributes
        self.sender = self.receiver = "m_signal_io"
        self.bbox_dir = "blackboxes"
        self.driver_path = self.comp_path = os.path.join(
            self.bbox_dir, self.module)
        self.WIDTH_DELIM = "//"

    @staticmethod
    def sig_fmt(fmt, split_func, array, delim=";\n    "):
        """Formats lists of signals based on fixed arguments

        Arguments:
            fmt {str} -- string format
            split_func {lambda/dict} -- map to split on the signals
            array {list(str)} -- list of signals
            delim {str} -- delimiter (default: {";n    "})

        Returns:
            {str} -- string formatted signals
        """
        return delim.join(fmt.format(**split_func(i)) for i in array)

    def __get_link_desc(self):

        return self.sig_fmt(
            """{{ "{link}", "{desc}", {{ "sst.Interfaces.StringEvent" }}}}""",
            lambda x: {
                "link": self.module + x[0],
                "desc": self.module + x[-1]
            },
            (("_din", " data in"), ("_dout", " data out")),
            ",\n" + " " * 8
        )

    def set_ports(self, ports):
        """Assigns ports to their corresponding member lists

        Arguments:
            ports {tuple(tuple(str,str,str),)} -- tuple of C++-style
                type-declared signals in the form
                ("<DTYPE>", "<PORT NAME>", "<PORT TYPE>"). The current types of
                signals supported are ("clock", "input", "output", "inout")
        """
        for port_dtype, port_name, port_type in ports:
            if port_type == "clock":
                self.clocks.append((port_dtype, port_name))
            elif port_type == "input":
                self.inputs.append((port_dtype, port_name))
            elif port_type == "output":
                self.outputs.append((port_dtype, port_name))
            elif port_type == "inout":
                self.inouts.append((port_dtype, port_name))
            else:
                raise ValueError("Each ports must be designated a type")

        self.ports = self.clocks + self.inputs + self.outputs + self.inouts
        self.ports = [(i[0].split(self.WIDTH_DELIM)[0], i[-1])
                      for i in self.ports]

    def _get_inputs(self, fmt, start_pos, signal_type_parser, splice=False,
                    clock_fmt=""):
        """Generates input bindings for both the components in the black box

        Arguments:
            driver {bool} -- option to generate code for the black box-driver
                (default: {True})

        Returns:
            {str} -- snippet of code representing input bindings
        """
        driver_inputs = []
        for driver_input in self.inputs:
            sig_len = signal_type_parser(driver_input[0])
            driver_inputs.append(
                fmt.format(
                    sp=start_pos,
                    sl=str(sig_len + (splice * start_pos)),
                    sig=driver_input[-1],
                )
            )
            start_pos += sig_len

        if self.clocks:
            for clock in self.clocks:
                driver_inputs.append(
                    clock_fmt.format(sp=start_pos, sig=clock[-1])
                )
                start_pos += int(clock[0].split(self.WIDTH_DELIM)[-1])

        self.buf_size = start_pos + 1 if splice else start_pos
        return ("\n" + " " * 8).join(driver_inputs)

    def __get_comp_defs(self):

        return {
            "lib_dir": self.lib_dir,
            "module": self.module,
            "lib": self.lib,
            "desc": self.desc,
            "ports": self.__get_link_desc(),
            "sig_type": self.sig_type,
            "buf_size": self.buf_size,
            "sender": self.sender,
            "receiver": self.receiver,
        }

    def __generate_comp_str(self):
        """Generates the black box-model code based on methods used to format
        the template file

        Returns:
            {str} -- boilerplate code representing the black box-model file
        """
        if os.path.isfile(self.comp_templ_path):
            with open(self.comp_templ_path) as template:
                return template.read().format(
                    **self.__get_comp_defs()
                )

        raise FileNotFoundError("Component boilerplate file not found")

    def __generate_driver_str(self):
        """Generates the black box-driver code based on methods used to format
        the template file

        Returns:
            {str} -- boilerplate code representing the black box-driver file
        """
        if os.path.isfile(self.drvr_templ_path):
            with open(self.drvr_templ_path) as template:
                return template.read().format(
                    inputs=self._get_driver_inputs(),
                    outputs=self._get_driver_outputs(),
                    **self._get_driver_defs()
                )

        raise FileNotFoundError("Driver boilerplate file not found")

    def _generate_driver(self):

        if self.driver_path:
            with open(self.driver_path, "w") as driver_file:
                driver_file.write(self.__generate_driver_str())

    def _generate_comp(self):

        if self.comp_path:
            with open(self.comp_path, "w") as comp_file:
                comp_file.write(self.__generate_comp_str())

    def generate_bbox(self):
        """Provides a high-level interface to the user to generate both the
        components of the black box and dump them to their corresponding files
        """
        if not len(self.ports):
            raise IndexError("Ports were not set properly")

        if not os.path.exists(self.bbox_dir):
            os.makedirs(self.bbox_dir)

        self._generate_driver()
        self._generate_comp()