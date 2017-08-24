<html>
<body> 
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
        echo "Saving descriptions... <br>";
        $Timestamp = $_POST['timeLine'];
        $Descriptions = $_POST['Description'];
        $numDescriptions = count($Descriptions);
        $numDescriptions = ($numDescriptions * 3);
        for($x=2; $x<$numDescriptions;){
			$theDescription = $Descriptions[$x]["description"];
			$theIP = $Descriptions[$x]["descriptionIP"];

			echo "$theIP : $theDescription <br>";
		
			$sqlInsert="INSERT OR REPLACE INTO descriptions (ID, ipAddress, ipDescription, ipTimestamp) VALUES ((SELECT ID FROM descriptions WHERE ipAddress = '$theIP'), '$theIP', '$theDescription', '$Timestamp');"; 

			$retInsert = $db->query($sqlInsert);
			$x+=3;
        }
		$db->close();
	?>
	<br>
</body>
</html>