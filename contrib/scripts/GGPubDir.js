// GG PubDir Search Show UIN
// version 0.2
// 2014-05-05
// written by Filip "widelec" Maryjanski (widelec@morphos.pl)
// public domain
//
// ==UserScript==
// @name          GG PubDir Search Show UIN
// @namespace     KwaKwa
// @description   Shows GG UIN in PubDir search on http://ipubdir.gadu-gadu.pl/ and http://ipubdir.gadu-gadu.pl/ngg/
// @include       http://ipubdir.gadu-gadu.pl/*
// @version       $VER: GG PubDir Search Show UIN 0.2 (05.05.2014)
// ==/UserScript==

function showUins(table, titleCellNo, dataCellNo)
{
	table.rows[0].cells[titleCellNo].innerHTML = 'NUMER GG';

	for(var i = 1, row; row = table.rows[i]; i++)
		table.rows[i].cells[dataCellNo].innerHTML = row.getElementsByTagName('a')[0].href.substring(3);
}

function addOnClick(divs)
{
	for(var i = 0; i < divs.length; i++)
	{
		var link = divs[i].getElementsByTagName('a')[0];

		if(link)
		{
			var oldOnClick = link.onclick;

			link.onclick = function()
			{
				oldOnClick();
				showUins(table, 4, 6);
			};
		}
	}
}

var t = document.getElementById('searchLightResults');

if(t != null)
{
	showUins(t, 8, 11);
}
else
{
	var table = document.getElementById("pubDirResults").getElementsByTagName('table')[0];

	addOnClick(document.getElementsByClassName('next'));
	addOnClick(document.getElementsByClassName('prev'));

	showUins(table, 4, 6);
}
