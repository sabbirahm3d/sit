package ram

import java.io.{File, InputStream, OutputStream}
import java.nio.charset.StandardCharsets.UTF_8
import scala.util.control.Breaks._

import org.newsclub.net.unix.{AFUNIXSocket, AFUNIXSocketAddress, AFUNIXServerSocket}
import chisel3.iotesters

class RAMUnitTester(c: RAM, args: Array[String]) extends iotesters.PeekPokeTester(c) {

    private val ram = c

    private val _sock: AFUNIXSocket = AFUNIXSocket.newInstance()
    _sock.connect(new AFUNIXSocketAddress(new File(args(0))))

    private val istream: InputStream = _sock.getInputStream()
    private val ostream: OutputStream = _sock.getOutputStream()

    ostream.write(ProcessHandle.current().pid().toString.getBytes("UTF-8"))
    ostream.flush()

    var _buf = new Array[Byte](10)

    breakable {

        while (true) {

            var read: Int = istream.read(_buf)
            var signal = new String(_buf, UTF_8)

            var alive: Int = signal.slice(0, 1).toInt
            signal = signal.slice(1, read)

            if (alive == 0) {
                break
            }

            var address: Int = signal.slice(0, 3).toInt
            var cs: Boolean = signal.slice(3, 4).toInt == 1
            var we: Boolean = signal.slice(4, 5).toInt == 1
            var oe: Boolean = signal.slice(5, 6).toInt == 1
            var data_in: Int = signal.slice(6, 9).toInt

            poke(ram.io.address, address)
            poke(ram.io.cs, cs)
            poke(ram.io.we, we)
            poke(ram.io.oe, oe)
            poke(ram.io.data_in, data_in)
            step(1)

            var data_out: String = peek(ram.io.data_out).toString

            ostream.write(data_out.getBytes("UTF-8"))
            ostream.flush()

        }
    }

    _buf = Array.empty[Byte]

}

object RAMMain extends App {
  iotesters.Driver.execute(args, () => new RAM) {
    c => new RAMUnitTester(c, args)
  }
}
