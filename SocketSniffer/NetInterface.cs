using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Text;
using System.Threading.Tasks;

namespace SocketSniffer
{
    internal class NetInterface
    {
        public static IEnumerable<IPAddress> GetAddrs()
        {
            var nics = NetworkInterface.GetAllNetworkInterfaces();
            foreach (var nic in nics)
            {
                foreach (var addr in nic.GetIPProperties().UnicastAddresses
                    .Select(a => a.Address))
                {
                    yield return addr;
                }
            }
        }
    }
}
