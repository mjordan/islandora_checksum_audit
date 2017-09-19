<?php

$path_to_foxml = trim($argv[1]);
$dom = new DOMDocument();
$dom->load($path_to_foxml);

$premis_entries = array();
foreach ($dom->getElementsByTagNameNS('info:fedora/fedora-system:def/audit#', 'record') as $record) {
  $id = $record->getAttributeNode('ID');
  foreach ($record->childNodes as $childNode) {
    if ($childNode->localName == 'date') {
      $date = $childNode->nodeValue;
    }
    if ($childNode->localName == 'justification') {
      $justification_string = $childNode->nodeValue;
    }

    if (isset($justification_string) && preg_match('/eventType=fixity\scheck/', $justification_string)) {
      $justification_details = parse_justification($justification_string);
      $premis_entries[$id->nodeValue]['dsid'] = $justification_details['file'];
      $premis_entries[$id->nodeValue]['date'] = $date;
      $premis_entries[$id->nodeValue]['event'] = $justification_details['event'];
    }

  }
}

function parse_justification($string) {
  $justification_parts = explode(';', $string);
  $file = explode('+', $justification_parts[0]);
  return array('file' => $file[1], 'event' => $justification_parts[2]);
}

print_r($premis_entries);
