package com.xyrlsz.opencc.android;

import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.github.houbb.opencc4j.util.ZhConverterUtil;
import com.xyrlsz.opencc.android.lib.ChineseConverter;
import com.xyrlsz.opencc.android.lib.ConversionType;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;


public class MainActivity extends AppCompatActivity {
    ExecutorService executorService = Executors.newSingleThreadExecutor();
    private ConversionType currentConversionType = ConversionType.TW2SP;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ChineseConverter.init(getApplicationContext());
        Spinner spinner = findViewById(R.id.spinner);
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(
                this, R.array.conversion_type_array, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);

        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                switch (position) {
                    case 0:
                        currentConversionType = ConversionType.TW2SP;
                        break;
                    case 1:
                        currentConversionType = ConversionType.S2HK;
                        break;
                    case 2:
                        currentConversionType = ConversionType.S2T;
                        break;
                    case 3:
                        currentConversionType = ConversionType.S2TW;
                        break;
                    case 4:
                        currentConversionType = ConversionType.S2TWP;
                        break;
                    case 5:
                        currentConversionType = ConversionType.T2HK;
                        break;
                    case 6:
                        currentConversionType = ConversionType.T2S;
                        break;
                    case 7:
                        currentConversionType = ConversionType.T2TW;
                        break;
                    case 8:
                        currentConversionType = ConversionType.TW2S;
                        break;
                    case 9:
                        currentConversionType = ConversionType.HK2S;
                        break;
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        final EditText textView = findViewById(R.id.text);

        findViewById(R.id.btn).setOnClickListener(v -> {
            String originalText = textView.getText().toString();
            Runnable runnable = () -> {
                final String converted = ChineseConverter.convert(
                        originalText, currentConversionType);
                textView.post(() -> textView.setText(converted));
            };
            executorService.execute(runnable);
        });
        findViewById(R.id.btn_test).setOnClickListener(v -> {
            String text = "我愛你";

            // opencc4j
            new Thread(() -> {
                Long start = System.currentTimeMillis();
                for (int i = 0; i < 10000; i++) {
                    ZhConverterUtil.toSimple(randomString(text));
                }
                Long end = System.currentTimeMillis();

                runOnUiThread(() -> {
                    Toast.makeText(MainActivity.this, "opencc4j耗时：" + (end - start), Toast.LENGTH_SHORT).show();
                });
            }).start();
            // opencc-android
            new Thread(() -> {
                Long start = System.currentTimeMillis();
                for (int i = 0; i < 10000; i++) {
                    ChineseConverter.convert(randomString(text), ConversionType.T2S);
                }
                Long end = System.currentTimeMillis();

                runOnUiThread(() -> {
                    Toast.makeText(MainActivity.this, "opencc-android耗时：" + (end - start), Toast.LENGTH_SHORT).show();
                });
            }).start();
        });
    }

    private String randomString(String input) {
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < input.length(); i++) {
            result.append(input.charAt(i));
            if (Math.random() < 0.5) {
                result.append(input.charAt(i));
            }
        }
        return result.toString();
    }
}
