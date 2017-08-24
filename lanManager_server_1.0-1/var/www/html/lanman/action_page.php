<html>
<body> 
	<?php
		$Installselection = $_POST['IP'];
		if(empty($Installselection)){
			echo "No clients were selected...";
		}
		else{
			$IPselection = count($Installselection);
			echo "$IPselection client(s) selected. <br>";
			for($i=0; $i<$IPselection; $i++){
				echo "<br>" . $Installselection[$i] . "<br>";
				exec("/var/www/lanman/lanManager_install.sh '$Installselection[$i]' 2>&1", $output);
				echo '<pre>';
				print_r($output);
				echo '</pre>';
				$output = array();
			}
		}
	?>
	<br>
</body>
</html>