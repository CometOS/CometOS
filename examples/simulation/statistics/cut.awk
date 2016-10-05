 {
  # cuts column after key match 
  # usage: pass key and offset via  -v key=$key -v offset=$offset 
   for(i=1;i<=NF;i++){
      if ($i==key)
          {print $(i+offset); break;}
   }
}
