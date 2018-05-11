<!DOCTYPE html>
<html>
<head>
	<title>FaceRecLock</title>
</head>

<body>

<?php

// Connect to the mySQL database

// Create connection
$con = mysqli_connect("localhost","root","","facerec");

// Check connection
if (mysqli_connect_errno($con))
  {
  echo "Failed to connect to MySQL: " . mysqli_connect_error();
  }
else
  {
  echo "mySQL status: Connected to Database <br /> <br />";
  }
  
  
//grad all the images that have not been checked yet
$result = mysqli_query($con, "SELECT img FROM newface WHERE checked = 0");

while($row = mysqli_fetch_array($result))
  {

  echo "<img src='" . $row['img'] . "' height='160' width='120'/>";
 //echo '<img src="data:image/jpeg;base64,'.base64_encode( $row['img'] ).'"/>';
  }
echo '<br>';
$dir = "images/*.png";
//get the list of all files with .jpg extension in the directory and save it in an array named $images
$images = glob( $dir );

//display each of those images
foreach( $images as $image ):
    echo "<img src='" . $image . "' height='160' width='120'/>";
endforeach;

  mysqli_close($con);
?>
	
<br> <br>
<input type="submit" class="button" name="unlock" value="unlock"/>

</body>
<script src="js/jquery.js" type="text/javascript"></script>
<script type="text/javascript">

 $('.button').click(function() {

 $.ajax({
  type: "POST",
  url: "unlock.php",
  data: { name: "Unlock" }
}).done(function( msg ) {
  alert( "Signal Sent: " + msg );
});    

    });

</script>

</html>