function request_backend(route) {
	const xhr = new XMLHttpRequest();
	const url = "https://iotmadnessproject.hopto.org:5000/" + route
	var data;
	xhr.onreadystatechange = function () {
		if (xhr.readyState == XMLHttpRequest.DONE) {
			data = xhr.response;
		}
	}
	xhr.open("GET", url, false);
	xhr.send();
	return JSON.parse(data);
}

function generateTable(plantID, filename) {
	var data = request_backend(`${plantID}/${filename}`);
	console.log(data);
	if (data.hasOwnProperty("list")) {
		data = data.list;
		let table = document.createElement('table');
		table.id = "dataTable";
		table.style.borderCollapse = "collapse";
		table.style.margin = "auto";
		table.style.width = "50%";
		let thead = document.createElement('thead');
		let tbody = document.createElement('tbody');
		let headRow = document.createElement('tr');
		headRow.style.backgroundColor = "lightgrey";
		headRow.style.color = "white";
		['Humidity %', 'Humidity Status', 'Water Level %', 'Water Level Status'].forEach(function (el) {
			let th = document.createElement('th');
			th.style.border = "1px solid black";
			th.style.padding = "10px";
			th.appendChild(document.createTextNode(el));
			headRow.appendChild(th);
		});
		thead.appendChild(headRow);
		table.appendChild(thead);
		data.forEach(function (el) {
			let tr = document.createElement('tr');
			['humidity', 'humidityStatus', 'waterLevel', 'waterLevelStatus'].forEach(function (prop) {
				let td = document.createElement('td');
				td.style.border = "1px solid black";
				td.style.padding = "10px";
				td.appendChild(document.createTextNode(el[prop]));
				tr.appendChild(td);
			});
			tbody.appendChild(tr);
		});
		table.appendChild(tbody);
		table.setAttribute('style', 'margin: auto');
		return table;
	}
}

function showDiv() {
	var data = request_backend(document.getElementById('plantIDFromUser').value);
	console.log(data);
	event.preventDefault();

	if (data.hasOwnProperty("list")) {
		document.getElementById("datesDropDownDiv").style.display = "block";

		const datesDropDown = document.getElementById("datesDropDown");
		const dateData = document.getElementById("dateData");
		datesDropDown.innerHTML = "";
		for (let key in data.list) {
			let option = document.createElement("option");
			option.setAttribute('value', data.list[key]);

			let optionText = document.createTextNode(data.list[key]);
			option.appendChild(optionText);

			datesDropDown.appendChild(option);
		}
	} else {
		alert(
			`${document.getElementById('plantIDFromUser').value} is not a recognized Plant ID, did you write it correctly?`
		);
	}

	datesDropDown.addEventListener("change", e => {
		const prevTable = document.getElementById('dataTable');
		if (prevTable != null) {
			prevTable.remove();
		}
		document.body.appendChild(generateTable(document.getElementById('plantIDFromUser').value, e.target.value));
	})
}

function sendWater() {
	var plantID = document.getElementById('plantIDFromUser').value;
	var data = request_backend(`water/${plantID}`);
	alert(`Successfully watered ${plantID}`);
	showDiv();
}

