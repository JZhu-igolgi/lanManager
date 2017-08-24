<!--
   Active IP page
 -->
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
   <a href="index.php" class="button" align="center">Overview</a>
   <a href="memory.php" class="button" align="center">Memory</a>
   <a href="disks.php" class="button">Disks</a>
   <a href="interfaces.php" class="button" align="center">Interfaces</a>
   <a href="boards.php" class="button">Boards</a>
   <a href="activeIP.php" class="button" align="center">Active IP</a>
</div>
<body>
   <div id="innerWrapper">
      <div id="innerWrapper">
      <a href="lanManager_client_1.0-1.deb" download="lanManager_client_1.0-1"><button class="download">Download</button></a>
      <h1>Local Area Network: Active IP</h1>
      <h3>About</h3>
      <p>
         This page offers a list of all currently active devices on you LAN, <br>
         this includes devices that have not installed the labManager program this website uses. <br>
         In the 'lanManager Client' column of the table, located below, <br>
         you can see which ip addresses have been registered as a known lanManager client system. 
      </p>
   <form id="install_form" action="action_page.php" onsubmit="window.open('action_page.php','popup','width=600,height=600,scrollbars=yes,resizable=yes');" target="popup" method="post">
      <input type="submit" value="Install on selected clients" class="install">
   </form>
   <form id="description_form" action="description_page.php" onsubmit="window.open('description_page.php','popup','width=600,height=600,scrollbars=yes,resizable=yes');" target="popup" method="post">
      <input type="submit" value="Save Descriptions" class="install">
   </form>
      <p><em>
         Refer to the navigational buttons near the top of the page, <br>
         for additional information, and graphs, about your LAN.
      </em></p>
      <div id = "tableBorder">
      <div id = "tableTitle">Active IP</div>
      <div class="tab">
         <button class="tablinks" onclick="openTable(event, '#1')" id="defaultOpen">Broadcast IP #1</button>
         <button class="tablinks" onclick="openTable(event, '#2')">Broadcast IP #2</button>
         <button class="tablinks" onclick="openTable(event, '#3')">Broadcast IP #3</button>
         <button class="tablinks" onclick="openTable(event, '#4')">Broadcast IP #4</button>
      </div>
      <div id="#1" class="tabcontent">
      <table style="width:90%" align="center">
         <tr>
            <th>IP Address</th>
            <th>Status</th>
            <th>lanManager Client</th>
            <th>Description</th>
            <th>Timestamp</th>
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
    
            $sqlCreateTable =<<<EOF
               CREATE TABLE IF NOT EXISTS descriptions(
                  ID INTEGER PRIMARY KEY AUTOINCREMENT,
                  ipAddress VARCHAR NOT NULL UNIQUE,
                  ipDescription VARCHAR NOT NULL,
                  ipTimestamp VARCHAR NOT NULL);
EOF;

            $retCreateTable = $db->query($sqlCreateTable);

            $sql =<<<EOF
               SELECT * FROM labInterfaces ORDER BY hostname;
EOF;
         $clientCheckVar = false;
         $descriptionCheckVar = false;
         $ipCounter = 0;

         $myfile = fopen("/etc/lanManager/lanScan.txt",
                            "r") or die("Unable to open file!");
         $timeLine = fgets($myfile);
         while(!feof($myfile)) {
            $ipLine = "";
            $ret = $db->query($sql);
            $curLine = fgets($myfile);
            if($curLine[0] == 'N' && $curLine[5] == 's'){
                  $strlen = strlen($curLine);
                  for($i = 0; $i < $strlen; $i++){
                     $curChar =  substr($curLine, $i, 1);
                     if(is_numeric($curChar) || $curChar == '.'){
                        $ipLine .= $curChar;
                     } elseif ($curChar == " ") {
                        $ipLine = "";
                     }
                  }
                  while($row = $ret->fetchArray(SQLITE3_ASSOC)) {
                     $ipVar = $row['ipAddress'];
                     if(strcmp($ipLine, $ipVar) == 0){
                        $clientCheckVar = true;
                     }
               }
            }

            $sqlDescriptions =<<<EOF
               SELECT * FROM descriptions WHERE ipAddress = '$ipLine';
EOF;
            $ret = $db->query($sqlDescriptions);
            $ipDescription = "";
            while($row = $ret->fetchArray(SQLITE3_ASSOC)) {
               $ipDescription = $row['ipDescription'];
            }
            if($clientCheckVar){
               echo "<tr>
                        <td>$ipLine</td>
                        <td>Active</td>
                        <td>
                           Known Client
                           <div class = \"status client\"></div>
                           <input form=\"install_form\" type=\"checkbox\" name=\"IP[]\" value=\"$ipLine\" class=\"IPcheckbox\">
                        </td>
                        <td><textarea form=\"description_form\" name=\"Description[$ipCounter][description]\" id=\"$ipLine\" rows=\"2\" cols=\"35\" placeholder=\"IP Address Description...\">$ipDescription</textarea></td>
                        <input form=\"description_form\" name=\"Description[$ipCounter][descriptionIP]\" value=\"$ipLine\" type=\"hidden\">
                        <input form=\"description_form\" name=\"timeLine\" value=\"$timeLine\" type=\"hidden\">
                        <td>$timeLine</td>
                     </tr>";
               $clientCheckVar = false;
            }
            else if($ipLine != "") {
               echo "<tr>
                        <td>$ipLine</td>
                        <td>Active</td>
                        <td>
                           Not Registered
                           <div class = \"status unknown\"></div>
                           <input form=\"install_form\" type=\"checkbox\" name=\"IP[]\" value=\"$ipLine\" class=\"IPcheckbox\">
                        </td>
                        <td><textarea form=\"description_form\" name=\"Description[$ipCounter][description]\" id=\"$ipLine\" rows=\"2\" cols=\"35\" placeholder=\"IP Address Description...\">$ipDescription</textarea></td>
                        <input form=\"description_form\" name=\"Description[$ipCounter][descriptionIP]\" value=\"$ipLine\" type=\"hidden\">
                        <input form=\"description_form\" name=\"timeLine\" value=\"$timeLine\" type=\"hidden\">
                        <td>$timeLine</td>
                     </tr>";
            }
            $ipCounter++;
         }
         fclose($myfile);
         $db->close();
      ?>
      </table>
      </div>

       <div id="#2" class="tabcontent">
         <table style="width:90%" align="center">
         <tr>
            <th>IP Address</th>
            <th>Status</th>
            <th>lanManager Client</th>
            <th>Description</th>
            <th>Timestamp</th>
         </tr>
      <?php
         $clientCheckVar = false;
         $descriptionCheckVar = false;
         $ipCounter = 0;

         $myfile = fopen("/etc/lanManager/lanScan2.txt",
                            "r") or die("Unable to open file!");
         $timeLine = fgets($myfile);
         while(!feof($myfile)) {
            $ipLine = "";
            $ret = $db->query($sql);
            $curLine = fgets($myfile);
            if($curLine[0] == 'N' && $curLine[5] == 's'){
                  $strlen = strlen($curLine);
                  for($i = 0; $i < $strlen; $i++){
                     $curChar =  substr($curLine, $i, 1);
                     if(is_numeric($curChar) || $curChar == '.'){
                        $ipLine .= $curChar;
                     } elseif ($curChar == " ") {
                        $ipLine = "";
                     }
                  }
                  while($row = $ret->fetchArray(SQLITE3_ASSOC)) {
                     $ipVar = $row['ipAddress'];
                     if(strcmp($ipLine, $ipVar) == 0){
                        $clientCheckVar = true;
                     }
               }
            }

            $sqlDescriptions =<<<EOF
               SELECT * FROM descriptions WHERE ipAddress = '$ipLine';
EOF;
            $ret = $db->query($sqlDescriptions);
            $ipDescription = "";
            while($row = $ret->fetchArray(SQLITE3_ASSOC)) {
               $ipDescription = $row['ipDescription'];
            }
            if($clientCheckVar){
               echo "<tr>
                        <td>$ipLine</td>
                        <td>Active</td>
                        <td>
                           Known Client
                           <div class = \"status client\"></div>
                           <input form=\"install_form\" type=\"checkbox\" name=\"IP[]\" value=\"$ipLine\" class=\"IPcheckbox\">
                        </td>
                        <td><textarea form=\"description_form\" name=\"Description[$ipCounter][description]\" id=\"$ipLine\" rows=\"2\" cols=\"35\" placeholder=\"IP Address Description...\">$ipDescription</textarea></td>
                        <input form=\"description_form\" name=\"Description[$ipCounter][descriptionIP]\" value=\"$ipLine\" type=\"hidden\">
                        <input form=\"description_form\" name=\"timeLine\" value=\"$timeLine\" type=\"hidden\">
                        <td>$timeLine</td>
                     </tr>";
               $clientCheckVar = false;
            }
            else if($ipLine != "") {
               echo "<tr>
                        <td>$ipLine</td>
                        <td>Active</td>
                        <td>
                           Not Registered
                           <div class = \"status unknown\"></div>
                           <input form=\"install_form\" type=\"checkbox\" name=\"IP[]\" value=\"$ipLine\" class=\"IPcheckbox\">
                        </td>
                        <td><textarea form=\"description_form\" name=\"Description[$ipCounter][description]\" id=\"$ipLine\" rows=\"2\" cols=\"35\" placeholder=\"IP Address Description...\">$ipDescription</textarea></td>
                        <input form=\"description_form\" name=\"Description[$ipCounter][descriptionIP]\" value=\"$ipLine\" type=\"hidden\">
                        <input form=\"description_form\" name=\"timeLine\" value=\"$timeLine\" type=\"hidden\">
                        <td>$timeLine</td>
                     </tr>";
            }
            $ipCounter++;
         }
         fclose($myfile);
         $db->close();
      ?>
      </table>
      </div>

      <div id="#3" class="tabcontent">
         <table style="width:90%" align="center">
         <tr>
            <th>IP Address</th>
            <th>Status</th>
            <th>lanManager Client</th>
            <th>Description</th>
            <th>Timestamp</th>
         </tr>
      <?php
         $clientCheckVar = false;
         $descriptionCheckVar = false;
         $ipCounter = 0;

         $myfile = fopen("/etc/lanManager/lanScan3.txt",
                            "r") or die("Unable to open file!");
         $timeLine = fgets($myfile);
         while(!feof($myfile)) {
            $ipLine = "";
            $ret = $db->query($sql);
            $curLine = fgets($myfile);
            if($curLine[0] == 'N' && $curLine[5] == 's'){
                  $strlen = strlen($curLine);
                  for($i = 0; $i < $strlen; $i++){
                     $curChar =  substr($curLine, $i, 1);
                     if(is_numeric($curChar) || $curChar == '.'){
                        $ipLine .= $curChar;
                     } elseif ($curChar == " ") {
                        $ipLine = "";
                     }
                  }
                  while($row = $ret->fetchArray(SQLITE3_ASSOC)) {
                     $ipVar = $row['ipAddress'];
                     if(strcmp($ipLine, $ipVar) == 0){
                        $clientCheckVar = true;
                     }
               }
            }

            $sqlDescriptions =<<<EOF
               SELECT * FROM descriptions WHERE ipAddress = '$ipLine';
EOF;
            $ret = $db->query($sqlDescriptions);
            $ipDescription = "";
            while($row = $ret->fetchArray(SQLITE3_ASSOC)) {
               $ipDescription = $row['ipDescription'];
            }
            if($clientCheckVar){
               echo "<tr>
                        <td>$ipLine</td>
                        <td>Active</td>
                        <td>
                           Known Client
                           <div class = \"status client\"></div>
                           <input form=\"install_form\" type=\"checkbox\" name=\"IP[]\" value=\"$ipLine\" class=\"IPcheckbox\">
                        </td>
                        <td><textarea form=\"description_form\" name=\"Description[$ipCounter][description]\" id=\"$ipLine\" rows=\"2\" cols=\"35\" placeholder=\"IP Address Description...\">$ipDescription</textarea></td>
                        <input form=\"description_form\" name=\"Description[$ipCounter][descriptionIP]\" value=\"$ipLine\" type=\"hidden\">
                        <input form=\"description_form\" name=\"timeLine\" value=\"$timeLine\" type=\"hidden\">
                        <td>$timeLine</td>
                     </tr>";
               $clientCheckVar = false;
            }
            else if($ipLine != "") {
               echo "<tr>
                        <td>$ipLine</td>
                        <td>Active</td>
                        <td>
                           Not Registered
                           <div class = \"status unknown\"></div>
                           <input form=\"install_form\" type=\"checkbox\" name=\"IP[]\" value=\"$ipLine\" class=\"IPcheckbox\">
                        </td>
                        <td><textarea form=\"description_form\" name=\"Description[$ipCounter][description]\" id=\"$ipLine\" rows=\"2\" cols=\"35\" placeholder=\"IP Address Description...\">$ipDescription</textarea></td>
                        <input form=\"description_form\" name=\"Description[$ipCounter][descriptionIP]\" value=\"$ipLine\" type=\"hidden\">
                        <input form=\"description_form\" name=\"timeLine\" value=\"$timeLine\" type=\"hidden\">
                        <td>$timeLine</td>
                     </tr>";
            }
            $ipCounter++;
         }
         fclose($myfile);
         $db->close();
      ?>
      </table>
      </div>

      <div id="#4" class="tabcontent">
         <table style="width:90%" align="center">
         <tr>
            <th>IP Address</th>
            <th>Status</th>
            <th>lanManager Client</th>
            <th>Description</th>
            <th>Timestamp</th>
         </tr>
      <?php
         $clientCheckVar = false;
         $descriptionCheckVar = false;
         $ipCounter = 0;

         $myfile = fopen("/etc/lanManager/lanScan4.txt",
                            "r") or die("Unable to open file!");
         $timeLine = fgets($myfile);
         while(!feof($myfile)) {
            $ipLine = "";
            $ret = $db->query($sql);
            $curLine = fgets($myfile);
            if($curLine[0] == 'N' && $curLine[5] == 's'){
                  $strlen = strlen($curLine);
                  for($i = 0; $i < $strlen; $i++){
                     $curChar =  substr($curLine, $i, 1);
                     if(is_numeric($curChar) || $curChar == '.'){
                        $ipLine .= $curChar;
                     } elseif ($curChar == " ") {
                        $ipLine = "";
                     }
                  }
                  while($row = $ret->fetchArray(SQLITE3_ASSOC)) {
                     $ipVar = $row['ipAddress'];
                     if(strcmp($ipLine, $ipVar) == 0){
                        $clientCheckVar = true;
                     }
               }
            }

            $sqlDescriptions =<<<EOF
               SELECT * FROM descriptions WHERE ipAddress = '$ipLine';
EOF;
            $ret = $db->query($sqlDescriptions);
            $ipDescription = "";
            while($row = $ret->fetchArray(SQLITE3_ASSOC)) {
               $ipDescription = $row['ipDescription'];
            }
            if($clientCheckVar){
               echo "<tr>
                        <td>$ipLine</td>
                        <td>Active</td>
                        <td>
                           Known Client
                           <div class = \"status client\"></div>
                           <input form=\"install_form\" type=\"checkbox\" name=\"IP[]\" value=\"$ipLine\" class=\"IPcheckbox\">
                        </td>
                        <td><textarea form=\"description_form\" name=\"Description[$ipCounter][description]\" id=\"$ipLine\" rows=\"2\" cols=\"35\" placeholder=\"IP Address Description...\">$ipDescription</textarea></td>
                        <input form=\"description_form\" name=\"Description[$ipCounter][descriptionIP]\" value=\"$ipLine\" type=\"hidden\">
                        <input form=\"description_form\" name=\"timeLine\" value=\"$timeLine\" type=\"hidden\">
                        <td>$timeLine</td>
                     </tr>";
               $clientCheckVar = false;
            }
            else if($ipLine != "") {
               echo "<tr>
                        <td>$ipLine</td>
                        <td>Active</td>
                        <td>
                           Not Registered
                           <div class = \"status unknown\"></div>
                           <input form=\"install_form\" type=\"checkbox\" name=\"IP[]\" value=\"$ipLine\" class=\"IPcheckbox\">
                        </td>
                        <td><textarea form=\"description_form\" name=\"Description[$ipCounter][description]\" id=\"$ipLine\" rows=\"2\" cols=\"35\" placeholder=\"IP Address Description...\">$ipDescription</textarea></td>
                        <input form=\"description_form\" name=\"Description[$ipCounter][descriptionIP]\" value=\"$ipLine\" type=\"hidden\">
                        <input form=\"description_form\" name=\"timeLine\" value=\"$timeLine\" type=\"hidden\">
                        <td>$timeLine</td>
                     </tr>";
            }
            $ipCounter++;
         }
         fclose($myfile);
         $db->close();
      ?>
      </table>
      </div>
      </div>
     </div>
   </div>
<script>
   document.getElementById("defaultOpen").click();

   function openTable(evt, tableName) {
      var i, tabcontent, tablinks;
      tabcontent = document.getElementsByClassName("tabcontent");
      for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
      }
      tablinks = document.getElementsByClassName("tablinks");
      for (i = 0; i < tablinks.length; i++) {
         tablinks[i].className = tablinks[i].className.replace(" active", "");
      }
      document.getElementById(tableName).style.display = "block";
      evt.currentTarget.className += " active";
   }

</script>
</body>
<footer>
   For more on Igolgi products, go to: <a href="http://igolgi.com">www.igolgi.com</a> <br>
   Contact email: <a href="mailto:Eric.henricks@igolgi.com">Eric.henricks@igolgi.com</a><br>
   <em>Igolgi &copy; Copyright 2017</em>
</footer>
</div>
</html>