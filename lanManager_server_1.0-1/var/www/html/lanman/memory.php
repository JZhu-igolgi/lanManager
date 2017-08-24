<html id="pageWrapper" lang = "en-US">
<head>
   <meta charset = "UTF-8">
   <title>LAN MANAGER</title>
   <link rel="stylesheet" type="text/css" href="style.css">
</head>
<div id="borderWrapper" class="content">
<header>
   <img src="igolgi_banner.png" alt="Igolgi Banner">
   <em>LAN MANAGER</em>
</header>
<div id="buttonWrapper">
   <a href="index.php" class="button">Overview</a>
   <a href="memory.php" class="button">Memory</a>
   <a href="disks.php" class="button">Disks</a>
   <a href="interfaces.php" class="button">Interfaces</a>
   <a href="boards.php" class="button">Boards</a>
   <a href="activeIP.php" class="button">Active IP</a>
</div>
<body>
   <div id="innerWrapper">
      <div id="innerWrapper">
      <a href="lanManager_client_1.0-1.deb" download="lanManager_client_1.0-1"><button class="download">Download</button></a>
      <h1>Local Area Network: Memory</h1>
      <h3>About</h3>
      <p>
         This page offers a detailed listing of each client devices current memory status. <br>
         In the graph below, you will find a listing of all clients currently on the network, <br>
         with information about their hard disk, and ram memory.
      </p>
      <p><em>
         Refer to the navigational buttons near the top of the page, <br>
         for additional information, and graphs, about your LAN.
      </em></p>
      <div id = "tableBorder">
      <div id = "tableTitle">Memory</div>
      <table style="width:90%" align="center">
         <tr>
            <th>Hostname</th>
            <th>IP Address</th>
            <th>Total Memory</th>
            <th>Used Memory</th>
            <th>Free Memory</th>
            <th>Available Memory</th>
            <th>RAM Memory</th>
         </tr>
      <?php
            class MyDB extends SQLite3 {
               function __construct() {
                  $this->open('/etc/lanManager/packetTest.db');
               }
            }
            $db = new MyDB();
            if(!$db) {
               echo $db->lastErrorMsg();
            } else {
            }

            $sql =<<<EOF
               SELECT * FROM labAudit ORDER BY ipAddress;
EOF;

         $ret = $db->query($sql);
            while($row = $ret->fetchArray(SQLITE3_ASSOC) ) {
               $hostVar = $row['hostname'];
               $interfaceVar = $row['interface'];
               $ipVar = $row['ipAddress'];
               $cpuVar = $row['cpuModel'];
               $totalMemoryVar = $row['totalMemory'];
               $usedMemoryVar = $row['usedMemory'];
               $freeMemoryVar = $row['freeMemory'];
               $availableMemoryVar = $row['availableMemory'];
               $ramMemoryVar = $row['ramMemory'];
               echo "
               <tr>
                  <td>$hostVar </td>
                  <td>$ipVar</td>
                  <td>$totalMemoryVar</td>
                  <td>$usedMemoryVar</td>
                  <td>$freeMemoryVar</td>
                  <td>$availableMemoryVar</td>
                  <td>$ramMemoryVar</td>
               </tr> ";
            }
            $db->close();
      ?>
      </table>
      </div>
      </div>
   </div>
</body>
<footer>
   For more on Igolgi products, go to: <a href="http://igolgi.com">www.igolgi.com</a> <br>
   Contact email: <a href="mailto:Eric.henricks@igolgi.com">Eric.henricks@igolgi.com</a><br>
   <em>Igolgi &copy; Copyright 2017</em>
</footer>
</div>
</html>