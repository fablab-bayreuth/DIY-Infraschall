// WebSocket.js
// Define all Websocket stuff and command functions
// 
// Browser sends messages like "{"command":"save",...}
// ESP send messages like "{"event":"error", ... }

var t2000=new Date("2000-01-01T00:00:00.000Z")
var esp_time=0; //in UTC seconds since 2000
var esp_time_update = Date.now(); //JS-Time
var live_mode_start;
var live_mode_shift;
var live_mode_ts;
var live_mode_freq;
var live_mode=0; //store live mode

var connection = new WebSocket('ws://' + location.hostname + ':81/',
		[ 'arduino' ]);


arrAvg = function(arr){
	  var array_sum=arr.reduce(function(a,b){
	    return {y:a.y + b.y}
	  }, {y:0})
	  return array_sum.y/arr.length;
}

arrRMS = function(arr){
	var array_mean=arrAvg(arr)
	var square_sum=arr.reduce(function(a,b){
	    return {y:( a.y + (b.y-array_mean)**2 )}
	  }, {y:0});
	return (square_sum.y/arr.length)**(1/2)
}

function printTime(){

	var d= new Date();
	$('#computer_time').html(d.toLocaleString());
	var epoch=t2000.getTime()+1000*esp_time+(Date.now()-esp_time_update)
	var e=new Date(epoch);
	$('#esp_time').html(e.toLocaleString());	
}

function runner(){
  printTime();
  setTimeout(function() { runner(); }, 1000);
}


function setTime(){
	var msg = {
			command : "setTime",
			value : Math.floor((Date.now()-t2000.getTime())/1000),
			msec : (Date.now()-t2000.getTime())%1000,
	};
	console.log(msg);
	connection.send(JSON.stringify(msg))
}

connection.onopen = function() {
	//on open we could send the time to the ESP.
	//here we do nothing
	var msg = {
			command : "getAll",
	};
	console.log(msg);
	connection.send(JSON.stringify(msg))
};


connection.onerror = function(error) {
	console.log('WebSocket Error ', error);
};


connection.onmessage = function(e) {
	console.log('Server:'+e.data);
	var msg = JSON.parse(''+e.data);
	switch(msg.event){
	case "error":
		$('#error').html(msg.text)
		$('#error_tr').show();
		setTimeout(function(){
		    $('#error_tr').fadeOut(500)
		}, 3000)
		break
	case "msg":
		$('#msg').html(msg.text)
		$('#msg_tr').show();
		setTimeout(function(){
		    $('#msg_tr').fadeOut(500)
		}, 3000)
		break
	case "conf":
		$('#version').html(msg.version)
		$('#ssid').val(msg.ssid)
		$('#password').val(msg.password)
		$('#baud').val(msg.baud)
		$('#max_runtime').val(msg.max_runtime)
		$('#bat_factor').val(msg.bat_factor.toFixed(5))
		$('#bat_full').val(msg.bat_full.toFixed(2))
		$('#bat_empty').val(msg.bat_empty.toFixed(2))
		live_mode_freq=msg.live_freq
		$('#resolution').val(msg.resolution)
		$('#atmega').val(msg.atmega)
		$('#live_interval').val(msg.live_interval)
		$(".config").prop('disabled', false); //enable inputs
		setRF();
		break;
	case "time":
		esp_time=msg.time;
		esp_time_update=Date.now();
		printTime();
	break;
	case "download":
		if(msg.active){
			$(".file").prop('disabled', true);
			$(".config").prop('disabled', true);
			$(".clock").prop('disabled', true); 
			$("#logging").prop('disabled',true);
			$("#status").html("Download: "+msg.d_size+"/"+msg.f_size)
		} else {
			$(".file").prop('disabled', false);
			$(".config").prop('disabled', false);
			$(".clock").prop('disabled', false); 
			$("#logging").prop('disabled',false);
			if(msg.d_size==msg.f_size)
			  $("#status").html("Download completed: "+msg.d_size+"/"+msg.f_size)
 			else
			  $("#status").html("Download canceled")	
		}
		break;
	case "logging":
		if(msg.logging){
		$(".file").prop('disabled', true);
		$(".config").prop('disabled', true);
		$(".clock").prop('disabled', true); 
		$("#filename").val(msg.file);
		$("#runtime").val(msg.runtime);
		$("#logging").val("Stop Logging");
		$("#logging").css("color","#f00");
		$("#logging").css("font-weight","bold");
		$("#status").html("Size: "+msg.size+" - Time: "+msg.time)	
		} else {
		$(".file").prop('disabled', false);
		$(".config").prop('disabled', false);
		$(".clock").prop('disabled', false); 
		$("#logging").val("Start Logging");	
		$("#logging").css("color","#000");
		$("#logging").css("font-weight","normal");
		}
		$(".logging").prop('disabled', false); 
	break;
	case "setLive":
		if(! msg.live) {
			$('#live').val("Start live mode")
			$(".file").prop('disabled', false);
			$(".config").prop('disabled', false);
			$(".clock").prop('disabled', false); 			
			$(".logging").prop('disabled', false);
			live_mode=0;
		}
		$(".live").prop('disabled', false); 
	break;
	case "live":
		if(! live_mode){
			$('#live').val("Stop live mode")
			$(".file").prop('disabled', true);
			$(".config").prop('disabled', true);
			$(".clock").prop('disabled', true); 
			live_mode_start=Date.now()
			live_mode_shift=live_mode_freq*parseInt($('#live_interval').val())
			live_mode_ts=1000/live_mode_freq
			chart.series[0].remove();
			chart.addSeries({
			        name: 'Druck',
			        data: []
			    });
			$(".live").prop('disabled', false); 
			live_mode=1;
		}
		var values=msg.values.split(",")
		for (i = 0; i < values.length; i++) {
			v=parseFloat(values[i])
			shift = chart.series[0].data.length > live_mode_shift;
			chart.series[0].addPoint([ live_mode_start, v ], false, shift)
			live_mode_start+=live_mode_ts;
		}
		chart.redraw()
		$('#db_level').html((20*Math.log10(arrRMS(chart.series[0].data)/2e-5)).toFixed(1)+"dB(Z)")
		break
	case "bat":
		$('#bat').html(''+msg.value.toFixed(2)+'V')
		var percent=(msg.value-msg.empty)/(msg.full-msg.empty)
		if(percent>0.75) $('#bat_img').attr('src','bat1.gif')
		else if(percent>0.5) $('#bat_img').attr('src','bat2.gif')
		else if(percent>0.25) $('#bat_img').attr('src','bat3.gif')
		else $('#bat_img').attr('src','bat4.gif')
		break
	case "SDContent":
		$('#file_list').html("");
		msg.files.sort(function(a,b){
		    var nameA=a.n.toLowerCase(), nameB=b.n.toLowerCase()
		    if (nameA < nameB) //sort string descending
		        return 1 
		    if (nameA > nameB)
		        return -1
		    return 0 //default return value (no sorting)
		})
		for(var i=0;i<msg.files.length;i++){
			var s=msg.files[i].s;
			var s_dp=""+s+"b";
			if(s>1024){
			  s_dp=(1/1024*s).toPrecision(4)+'kb';
			}
			if(s>(1024*1024)){
			  s_dp=(1/1024/1024*s).toPrecision(4)+'mb';
			}
			$('#file_list').append("<tr><td>"+msg.files[i].n+"</td>"+
"<td><input class=\"file\" type=\"button\" value=\""+s_dp+"\" onClick=\"downloadFile('"+msg.files[i].n+"')\"></td>"+
"<td><input class=\"file\" type=\"button\" value=\"delete\" onClick=\"if(confirm('Are you sure?')) deleteFile('"+msg.files[i].n+"')\"></td>"+
"</tr>")
		}	
	break;
	//TODO: add further events to handle
	}
};

connection.onclose = function() {
	console.log('WebSocket connection closed');
};


function setLogging(){

	if($("#logging").val()=="Start Logging"){
	   var runtime=parseInt($('#runtime').val())
	   if(isNaN(runtime) || runtime<=0) runtime=parseInt($('#max_runtime').val())
	   msg = { command : "startLogging",file: $('#filename').val(),runtime: runtime }
	} else {
	   msg = { command : "stopLogging" }
        }
	console.log(msg);
	connection.send(JSON.stringify(msg));
	$(".logging").prop('disabled', true); 	
	
}

//Start live mode
function setLive(){
	if($("#live").val()=="Start live mode"){
		   msg = { command : "startLive" }
	} else {
		   msg = { command : "stopLive" }
	}
	console.log(msg);
	connection.send(JSON.stringify(msg));
	$(".logging").prop('disabled', true); 	
	$(".live").prop('disabled', true); 	
}

//set Options from frequency/resolution and board type
function setRF(){
	var atmega = parseInt($('#atmega').val())
	$("#resolution > option").each(function() {
		var o=$('#resolution > option[value="'+this.value+'"]')
		if(atmega){
			if(this.value=='2') o.text("12 bit, 142.8571 Hz")
			else if(this.value=='3') o.text("13 bit, 90.9091 Hz")
			else if(this.value=='4') o.text("14 bit, 50 Hz")
			else if(this.value=='5') o.text("15 bit, 25 Hz")
		} else {
			if(this.value=='2') o.text("12 bit, 151.7037 Hz")
			else if(this.value=='3') o.text("13 bit, 91.0222 Hz")
			else if(this.value=='4') o.text("14 bit, 51.8481 Hz")
			else if(this.value=='5') o.text("15 bit, 25.92405 Hz")			
		}
	})
}

//Save config to EEPROM
function saveConf() {
	var max_runtime = parseInt($('#max_runtime').val())
	var baud = parseInt($('#baud').val())
	var bat_factor = parseFloat($('#bat_factor').val())
	var bat_full = parseFloat($('#bat_full').val())
	var bat_empty = parseFloat($('#bat_empty').val())
	var atmega = parseInt($('#atmega').val())
	var resolution = parseInt($('#resolution').val())
	var live_interval = parseInt($('#live_interval').val())
	if(isNaN(live_interval)) live_interval=10;

	if (isNaN(baud) || isNaN(max_runtime) || isNaN(bat_factor) || isNaN(bat_full) || isNaN(bat_empty) ) {
		alert("Baud, Max Runtime and Battery config must be a number!")
		return
	}
	var msg = {
			command : "setConf",
			ssid : $('#ssid').val(),
			password : $('#password').val(),
			baud : baud,
			max_runtime : max_runtime,
			bat_factor: bat_factor,
			bat_full: bat_full,
			bat_empty: bat_empty,
			resolution: resolution,
			atmega: atmega,
			live_interval: live_interval
		};
	console.log(msg);
	connection.send(JSON.stringify(msg));
	$(".config").prop('disabled', true);		
}

//Delete File
function deleteFile(file){
	var msg = {
			command : "delFile",
			file : file
		};
	console.log(msg);
	connection.send(JSON.stringify(msg));
}

//Start Download
function downloadFile(file){
	var url='/download?f='+file;
	var _iframe_dl = $('<iframe />')
	       .attr('src', url)
	       .hide()
	       .appendTo('body');	
}

