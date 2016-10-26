package com.kappa.aapt;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;


public class MainActivity extends AppCompatActivity {

    private final int FILE_SELECT_CODE = 1001;
    private TextView pathView;
    private TextView infoView;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        setTitle(R.string.title_name);
        Toolbar toolbar = (Toolbar) findViewById(R.id.activity_main_toolbar);
        setSupportActionBar(toolbar);
        toolbar.setLogo(R.mipmap.ic_launcher);

        pathView = (TextView) findViewById(R.id.title_path_txv);

        Button btn = (Button) findViewById(R.id.title_open_btn);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                intent.setType("*/*");
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                try {
                    startActivityForResult(Intent.createChooser(intent, getText(R.string.tips_select_apk)), FILE_SELECT_CODE);
                } catch (android.content.ActivityNotFoundException ex) {
                    Toast.makeText(MainActivity.this, getText(R.string.tips_no_fileselector), Toast.LENGTH_SHORT).show();
                }
            }
        });

        infoView = (TextView) findViewById(R.id.sample_text);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == FILE_SELECT_CODE) {
            if (resultCode == Activity.RESULT_OK) {
                Uri url = data.getData();
                pathView.setText(url.getPath());
                infoView.setText(getApkInfo(url.getPath()));
            }
        }

        super.onActivityResult(requestCode, resultCode, data);
    }

    public native String getApkInfo(String path);
}
