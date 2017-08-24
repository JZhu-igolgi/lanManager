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
      <h1>Local Area Network: Boards</h1>
      <h3>About</h3>
      <p>
         This page offers a detailed listing of each client devices current deltacast board status. <br>
         In the graph below, you will find a listing of most clients currently on the network, <br>
         with information about all of their known deltacast PCI boards. <br><br>
         **NOTE** Only clients that can run the "deltcast_board_info" script, and have detectable boards, <br>
                  will appear in the table below. If you don't see a client appearing in the table that should be there, <br>
                  then please refer to the "lanManager_client_log.txt" document located in the "/var/log/lanManager" directory.
      </p>
      <p><em>
         Refer to the navigational buttons near the top of the page, <br>
         for additional information, and graphs, about your LAN.
      </em></p>
      <div id = "tableBorder">
      <div id = "tableTitle">Disks</div>
      <table style="width:90%" align="center">
         <tr>
            <th>Hostname</th>
            <th>IP Address</th>
            <th>Script Output</th>
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
               SELECT * FROM labBoards ORDER BY ipAddress;
EOF;

         $numBoardsVar = 1;
         $ret = $db->query($sql);
            while($row = $ret->fetchArray(SQLITE3_ASSOC) ) {
               $hostVar = $row['hostname'];
               $ipVar = $row['ipAddress'];
               $scriptVar = $row['scriptOutput'];
               if($scriptVar != ""){
                  echo "
                  <tr>
                     <td>$hostVar </td>
                     <td>$ipVar</td>
                     <td><pre>$scriptVar</pre></td>
                  </tr> ";
               } else {
                  echo "
                  <tr>
                     <td>$hostVar </td>
                     <td>$ipVar</td>
                     <td>Failed to run script: Library libvideomasterhd.so.1 may be missing <br>
                           Please check client log for additional information.</td>
                  </tr> ";
               }
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